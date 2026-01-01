#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include <QMutex>
#include <QtDebug>
#include <QNetworkProxyFactory>
#include <QPalette>
#include <QFont>
#include <QCursor>
#include <QElapsedTimer>
#include <QTemporaryFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QFile>

#ifdef Q_OS_UNIX
#include <sys/socket.h>
#include <signal.h>
#endif

// Don't let SDL hook our main function, since Qt is already
// doing the same thing. This needs to be before any headers
// that might include SDL.h themselves.
#define SDL_MAIN_HANDLED
#include "SDL_compat.h"

#ifdef HAVE_FFMPEG
#include "streaming/video/ffmpeg.h"
#endif

#if defined(Q_OS_WIN32)
#include "antihookingprotection.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(Q_OS_LINUX)
#include <openssl/ssl.h>
#endif

#include "cli/listapps.h"
#include "cli/quitstream.h"
#include "cli/startstream.h"
#include "cli/pair.h"
#include "cli/commandlineparser.h"
#include "path.h"
#include "utils.h"
#include "gui/computermodel.h"
#include "gui/appmodel.h"
#include "backend/autoupdatechecker.h"
#include "backend/computermanager.h"
#include "backend/systemproperties.h"
#include "streaming/session.h"
#include "settings/streamingpreferences.h"
#include "gui/sdlgamepadkeynavigation.h"
#include "backend/pemhttpclient.h"
#include "path.h"

#if defined(Q_OS_WIN32)
#define IS_UNSPECIFIED_HANDLE(x) ((x) == INVALID_HANDLE_VALUE || (x) == NULL)

// Log to file or console dynamically for Windows builds
#define LOG_TO_FILE
#elif !defined(QT_DEBUG) && defined(Q_OS_DARWIN)
// Log to file for release Mac builds
#define LOG_TO_FILE
#else
// Log to console for debug Mac builds
#endif

// StreamUtils::setAsyncLogging() exposes control of this to the Session
// class to enable async logging once the stream has started.
//
// FIXME: Clean this up
QAtomicInt g_AsyncLoggingEnabled;

static QElapsedTimer s_LoggerTime;
static QTextStream s_LoggerStream;
static QThreadPool s_LoggerThread;
static QMutex s_SyncLoggerMutex;
static bool s_SuppressVerboseOutput;
static QRegularExpression k_RikeyRegex("&rikey=\\w+");
static QRegularExpression k_RikeyIdRegex("&rikeyid=[\\d-]+");
#ifdef LOG_TO_FILE
// Max log file size of 10 MB
static const uint64_t k_MaxLogSizeBytes = 10 * 1024 * 1024;
static QAtomicInteger<uint64_t> s_LogBytesWritten = 0;
static QFile* s_LoggerFile = nullptr;
#endif

class LoggerTask : public QRunnable
{
public:
    LoggerTask(const QString& msg) : m_Msg(msg)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        // QTextStream is not thread-safe, so we must lock. This will generally
        // only contend in synchronous logging mode or during a transition
        // between synchronous and asynchronous. Asynchronous won't contend in
        // the common case because we only have a single logging thread.
        QMutexLocker locker(&s_SyncLoggerMutex);
        s_LoggerStream << m_Msg;
        s_LoggerStream.flush();
    }

private:
    QString m_Msg;
};

void logToLoggerStream(QString& message)
{
#if defined(QT_DEBUG) && defined(Q_OS_WIN32)
    // Output log messages to a debugger if attached
    if (IsDebuggerPresent()) {
        thread_local QString lineBuffer;
        lineBuffer += message;
        if (message.endsWith('\n')) {
            OutputDebugStringW(lineBuffer.toStdWString().c_str());
            lineBuffer.clear();
        }
    }
#endif

    // Strip session encryption keys and IVs from the logs
    message.replace(k_RikeyRegex, "&rikey=REDACTED");
    message.replace(k_RikeyIdRegex, "&rikeyid=REDACTED");

#ifdef LOG_TO_FILE
    auto oldLogSize = s_LogBytesWritten.fetchAndAddRelaxed(message.size());
    if (oldLogSize >= k_MaxLogSizeBytes) {
        return;
    }
    else if (oldLogSize >= k_MaxLogSizeBytes - message.size()) {
        // Write one final message
        message = "Log size limit reached!";
    }
#endif

    if (g_AsyncLoggingEnabled) {
        // Queue the log message to be written asynchronously
        s_LoggerThread.start(new LoggerTask(message));
    }
    else {
        // Log the message immediately
        LoggerTask(message).run();
    }
}

void sdlLogToDiskHandler(void*, int category, SDL_LogPriority priority, const char* message)
{
    QString priorityTxt;

    switch (priority) {
    case SDL_LOG_PRIORITY_VERBOSE:
        if (s_SuppressVerboseOutput) {
            return;
        }
        priorityTxt = "Verbose";
        break;
    case SDL_LOG_PRIORITY_DEBUG:
        if (s_SuppressVerboseOutput) {
            return;
        }
        priorityTxt = "Debug";
        break;
    case SDL_LOG_PRIORITY_INFO:
        if (s_SuppressVerboseOutput) {
            return;
        }
        priorityTxt = "Info";
        break;
    case SDL_LOG_PRIORITY_WARN:
        if (s_SuppressVerboseOutput) {
            return;
        }
        priorityTxt = "Warn";
        break;
    case SDL_LOG_PRIORITY_ERROR:
        priorityTxt = "Error";
        break;
    case SDL_LOG_PRIORITY_CRITICAL:
        priorityTxt = "Critical";
        break;
    default:
        priorityTxt = "Unknown";
        break;
    }

    QTime logTime = QTime::fromMSecsSinceStartOfDay(s_LoggerTime.elapsed());
    QString txt = QString("%1 - SDL %2 (%3): %4\n").arg(logTime.toString()).arg(priorityTxt).arg(category).arg(message);

    logToLoggerStream(txt);
}

void qtLogToDiskHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    QString typeTxt;

    switch (type) {
    case QtDebugMsg:
        if (s_SuppressVerboseOutput) {
            return;
        }
        typeTxt = "Debug";
        break;
    case QtInfoMsg:
        if (s_SuppressVerboseOutput) {
            return;
        }
        typeTxt = "Info";
        break;
    case QtWarningMsg:
        if (s_SuppressVerboseOutput) {
            return;
        }
        typeTxt = "Warning";
        break;
    case QtCriticalMsg:
        typeTxt = "Critical";
        break;
    case QtFatalMsg:
        typeTxt = "Fatal";
        break;
    default:
        typeTxt = "Unknown";
        break;
    }

    QTime logTime = QTime::fromMSecsSinceStartOfDay(s_LoggerTime.elapsed());
    QString txt = QString("%1 - Qt %2: %3\n").arg(logTime.toString()).arg(typeTxt).arg(msg);

    logToLoggerStream(txt);
}

// 专门处理QML日志的函数
void qmlLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QTime logTime = QTime::fromMSecsSinceStartOfDay(s_LoggerTime.elapsed());
    QString prefix;
    
    switch (type) {
    case QtDebugMsg:
        prefix = "QML Debug";
        break;
    case QtInfoMsg:
        prefix = "QML Info";
        break;
    case QtWarningMsg:
        prefix = "QML Warning";
        break;
    case QtCriticalMsg:
        prefix = "QML Critical";
        break;
    case QtFatalMsg:
        prefix = "QML Fatal";
        break;
    default:
        prefix = "QML Unknown";
        break;
    }

    QString txt = QString("%1 - %2: %3\n").arg(logTime.toString()).arg(prefix).arg(msg);
    logToLoggerStream(txt);
}

#ifdef Q_OS_WIN32
static HANDLE k_StandardHandles[] = {
    GetStdHandle(STD_INPUT_HANDLE),
    GetStdHandle(STD_OUTPUT_HANDLE),
    GetStdHandle(STD_ERROR_HANDLE)
};

// This is called when we're about to execute a new process via QProcess
static void resetStdioHandlesForQProcess(void)
{
    for (auto handle : k_StandardHandles) {
        if (IS_UNSPECIFIED_HANDLE(handle)) {
            continue;
        }

        if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
            qWarning() << "SetHandleInformation() failed:" << GetLastError();
        }
    }
}
#endif

static void initializeStandardStreams()
{
    // Ensure we have console handles available for logging
#if defined(Q_OS_WIN32) && !defined(QT_DEBUG)
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        // Create a new console if we're not launched from one
        AllocConsole();
    }

    // Ensure stdio handles are inherited by child processes
    // This is required for streaming to work properly
    for (auto handle : k_StandardHandles) {
        if (IS_UNSPECIFIED_HANDLE(handle)) {
            continue;
        }

        if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
            qWarning() << "SetHandleInformation() failed:" << GetLastError();
        }
    }

    // Register the handle reset function with QProcess
    qputenv("QT_QPA_ENABLE_TERMINAL_INPUT", "1");
    QProcess::setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        resetStdioHandlesForQProcess();
    });
#endif

    // Initialize logging streams
    s_LoggerTime.start();
    // 在Qt 6中，QTextStream默认使用UTF-8编码，所以不再需要显式设置
    // 或者可以使用QTextCodec::codecForName("UTF-8")，但需要包含Qt Core Addons

#ifdef LOG_TO_FILE
    // 创建日志文件在当前目录
    QString logPath = "moonlight-debug.log";
    s_LoggerFile = new QFile(logPath);
    if (s_LoggerFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        s_LoggerStream.setDevice(s_LoggerFile);
        qInfo() << "Logging to file:" << s_LoggerFile->fileName();
        qInfo() << "日志系统初始化成功";
    }
    else {
        qWarning() << "Failed to create log file. Logging to stderr instead";
        // 在Qt 6中，QTextStream不能直接使用stderr，需要创建QFile对象
        static QFile stderrFile;
        stderrFile.open(stderr, QIODevice::WriteOnly);
        s_LoggerStream.setDevice(&stderrFile);
        delete s_LoggerFile;
        s_LoggerFile = nullptr;
    }
#else
    // 在Qt 6中，QTextStream不能直接使用stderr，需要创建QFile对象
    static QFile stderrFile;
    stderrFile.open(stderr, QIODevice::WriteOnly);
    s_LoggerStream.setDevice(&stderrFile);
#endif

    // Hook up Qt logging
    qInstallMessageHandler(qtLogToDiskHandler);

    // Hook up QML logging
    qInstallMessageHandler(qmlLogHandler);

    // Hook up SDL logging
    SDL_LogSetOutputFunction(sdlLogToDiskHandler, nullptr);
    
    // 输出初始化日志
    qInfo() << "Qt日志系统初始化完成";
    qInfo() << "QML日志系统初始化完成";
}

// We need to be careful to not log anything before this is called!
static void initializeAsyncLogging()
{
    Q_ASSERT(g_AsyncLoggingEnabled == 0);

    // Start async logging thread
    s_LoggerThread.setExpiryTimeout(30000);
    // 在Qt 6中，QThreadPool不需要手动启动，它会自动管理线程

    // Enable async logging
    g_AsyncLoggingEnabled.ref();
}

static void restoreSdlLogFunction()
{
    // This function is called after Qt's logging is shut down.
    // We must restore SDL's default logging function to ensure
    // console output continues to work for other libraries.
    SDL_LogSetOutputFunction(nullptr, nullptr); // 使用nullptr而不是SDL_GetDefaultLogOutputFunction()
}

// This is a Qt/C++ equivalent of the Java/C# finally construct
class Finally
{
public:
    Finally(std::function<void()> f) : finallyFunc(f) {}
    ~Finally() { finallyFunc(); }
private:
    std::function<void()> finallyFunc;
};

int main(int argc, char *argv[])
{
    // Initialize our logging before anything else
    initializeStandardStreams();

    // Initialize paths
    Path::initialize(false);

    // Ensure we restore SDL's default logging function on exit
    Finally finally([]() { restoreSdlLogFunction(); });

    QCoreApplication::addLibraryPath(".");

    QGuiApplication app(argc, argv);

    // Set application attributes
    app.setAttribute(Qt::AA_ShareOpenGLContexts);

    // Set application metadata
    app.setApplicationName("Moonlight");
    app.setApplicationVersion(VERSION_STR);
    app.setOrganizationName("Moonlight Game Streaming Project");
    app.setOrganizationDomain("moonlight-stream.org");

    // On Linux, set the desktop file name to allow the window manager to
    // match our windows to our desktop file for proper theming.
    app.setDesktopFileName("com.moonlight_stream.Moonlight");
    qputenv("SDL_VIDEO_WAYLAND_WMCLASS", "com.moonlight_stream.Moonlight");
    qputenv("SDL_VIDEO_X11_WMCLASS", "com.moonlight_stream.Moonlight");

    // Register our C++ types for QML
    qmlRegisterType<ComputerModel>("ComputerModel", 1, 0, "ComputerModel");
    qmlRegisterType<AppModel>("AppModel", 1, 0, "AppModel");
    qmlRegisterUncreatableType<Session>("Session", 1, 0, "Session", "Session cannot be created from QML");
    qmlRegisterSingletonType<ComputerManager>("ComputerManager", 1, 0,
                                              "ComputerManager",
                                              [](QQmlEngine* qmlEngine, QJSEngine*) -> QObject* {
                                                  return new ComputerManager(StreamingPreferences::get(qmlEngine));
                                              });
    qmlRegisterSingletonType<AutoUpdateChecker>("AutoUpdateChecker", 1, 0,
                                                "AutoUpdateChecker",
                                                [](QQmlEngine*, QJSEngine*) -> QObject* {
                                                    return new AutoUpdateChecker();
                                                });
    qmlRegisterSingletonType<SystemProperties>("SystemProperties", 1, 0,
                                               "SystemProperties",
                                               [](QQmlEngine*, QJSEngine*) -> QObject* {
                                                   return new SystemProperties();
                                               });
    qmlRegisterSingletonType<SdlGamepadKeyNavigation>("SdlGamepadKeyNavigation", 1, 0,
                                                      "SdlGamepadKeyNavigation",
                                                      [](QQmlEngine* qmlEngine, QJSEngine*) -> QObject* {
                                                          return new SdlGamepadKeyNavigation(StreamingPreferences::get(qmlEngine));
                                                      });
    qmlRegisterSingletonType<StreamingPreferences>("StreamingPreferences", 1, 0,
                                                   "StreamingPreferences",
                                                   [](QQmlEngine* qmlEngine, QJSEngine*) -> QObject* {
                                                       return StreamingPreferences::get(qmlEngine);
                                                   });
    // 注册PemHttpClient为单例类型
    qmlRegisterSingletonType<PemHttpClient>("PemHttpClient", 1, 0,
                                           "PemHttpClient",
                                           [](QQmlEngine*, QJSEngine*) -> QObject* {
                                               return new PemHttpClient();
                                           });

    // Create the identity manager on the main thread
    IdentityManager::get();

    // We require the Material theme
    QQuickStyle::setStyle("Material");

    // Our icons are styled for a dark theme, so we do not allow the user to override this
    qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", "Dark");

    // These are defaults that we allow the user to override
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_MATERIAL_ACCENT")) {
        qputenv("QT_QUICK_CONTROLS_MATERIAL_ACCENT", "Purple");
    }
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_MATERIAL_VARIANT")) {
        qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");
    }
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_MATERIAL_PRIMARY")) {
        // Qt 6.9 began to use a different shade of Material.Indigo when we use a dark theme
        // (which is all the time). The new color looks washed out, so manually specify the
        // old primary color unless the user overrides it themselves.
        qputenv("QT_QUICK_CONTROLS_MATERIAL_PRIMARY", "#3F51B5");
    }

    QQmlApplicationEngine engine;
    QString initialView;
    bool hasGUI = true;

    // 声明commandLineParserResult变量，使用正确的枚举类型
    GlobalCommandLineParser::ParseResult commandLineParserResult = GlobalCommandLineParser().parse(app.arguments());

    switch (commandLineParserResult) {
    case GlobalCommandLineParser::NormalStartRequested:
        initialView = "qrc:/gui/PcView.qml";
        break;
    case GlobalCommandLineParser::StreamRequested:
        {
            initialView = "qrc:/gui/CliStartStreamSegue.qml";
            StreamingPreferences* preferences = StreamingPreferences::get();
            StreamCommandLineParser streamParser;
            streamParser.parse(app.arguments(), preferences);
            QString host    = streamParser.getHost();
            QString appName = streamParser.getAppName();
            auto launcher   = new CliStartStream::Launcher(host, appName, preferences, &app);
            engine.rootContext()->setContextProperty("launcher", launcher);
            break;
        }
    case GlobalCommandLineParser::QuitRequested:
        {
            initialView = "qrc:/gui/CliQuitStreamSegue.qml";
            QuitCommandLineParser quitParser;
            quitParser.parse(app.arguments());
            auto launcher = new CliQuitStream::Launcher(quitParser.getHost(), &app);
            engine.rootContext()->setContextProperty("launcher", launcher);
            break;
        }
    case GlobalCommandLineParser::PairRequested:
        {
            initialView = "qrc:/gui/CliPair.qml";
            PairCommandLineParser pairParser;
            pairParser.parse(app.arguments());
            auto launcher = new CliPair::Launcher(pairParser.getHost(), pairParser.getPredefinedPin(), &app);
            engine.rootContext()->setContextProperty("launcher", launcher);
            break;
        }
    case GlobalCommandLineParser::ListRequested:
        {
            ListCommandLineParser listParser;
            listParser.parse(app.arguments());
            auto launcher = new CliListApps::Launcher(listParser.getHost(), listParser, &app);
            launcher->execute(new ComputerManager(StreamingPreferences::get()));
            hasGUI = false;
            break;
        }
    }

    if (hasGUI) {
        engine.rootContext()->setContextProperty("initialView", initialView);

        // Load the main.qml file
        engine.load(QUrl(QStringLiteral("qrc:/gui/main.qml")));
        if (engine.rootObjects().isEmpty())
            return -1;
    }

    int err = app.exec();

    // Give worker tasks time to properly exit. Fixes PendingQuitTask
    // sometimes freezing and blocking process exit.
    QThreadPool::globalInstance()->waitForDone(30000);

    // Restore the default logger for all libraries before shutting down ours
    SDL_LogSetOutputFunction(nullptr, nullptr); // 使用nullptr而不是oldSdlLogFn
    qInstallMessageHandler(nullptr);
#ifdef HAVE_FFMPEG
    av_log_set_callback(nullptr); // 使用nullptr而不是av_log_default_callback
#endif

    // We should not be in async logging mode anymore
    Q_ASSERT(g_AsyncLoggingEnabled == 0);

    // Wait for pending log messages to be printed
    s_LoggerThread.waitForDone();

#ifdef Q_OS_WIN32
    // Without an explicit flush, console redirection for the list command
    // doesn't work reliably (sometimes the target file contains no text).
    fflush(stderr);
    fflush(stdout);
#endif

    return err;
}