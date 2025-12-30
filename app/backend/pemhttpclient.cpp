#include "pemhttpclient.h"
#include <QFile>
#include <QLoggingCategory>
#include <QSslSocket>
#include <QEventLoop>
#include <QTimer>
#include <QMetaObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QDir>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(pemHttpClient, "pem.http.client")

PemHttpClient::PemHttpClient(QObject *parent)
    : QObject(parent)
{
    m_baseUrl = "https://cloudgame-test.mingland.cn/prod-api";
    
    qCInfo(pemHttpClient) << "正在配置HTTPS客户端，目标URL:" << m_baseUrl;

    if (!loadCertificateAndKey()) {
        qCCritical(pemHttpClient) << "无法加载证书和私钥";
        return;
    }

    m_networkManager = std::make_unique<QNetworkAccessManager>(this);
}

PemHttpClient::~PemHttpClient()
{
}

bool PemHttpClient::loadCertificateAndKey()
{
    const QString clientCertPath = ":/Resources/fullchain.crt";
    const QString clientKeyPath = ":/Resources/client.key";

    qCInfo(pemHttpClient) << "正在加载证书:" << clientCertPath;
    qCInfo(pemHttpClient) << "正在加载私钥:" << clientKeyPath;

    // 读取证书文件
    QFile certFile(clientCertPath);
    if (!certFile.open(QIODevice::ReadOnly)) {
        qCCritical(pemHttpClient) << "无法打开证书文件:" << certFile.errorString();
        return false;
    }
    QByteArray certData = certFile.readAll();
    m_clientCertificate = QSslCertificate(certData, QSsl::Pem);

    if (m_clientCertificate.isNull()) {
        qCCritical(pemHttpClient) << "加载证书失败，证书为空";
        return false;
    }

    // 读取私钥文件
    QFile keyFile(clientKeyPath);
    if (!keyFile.open(QIODevice::ReadOnly)) {
        qCCritical(pemHttpClient) << "无法打开私钥文件:" << keyFile.errorString();
        return false;
    }
    QByteArray keyData = keyFile.readAll();
    m_privateKey = QSslKey(keyData, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);

    if (m_privateKey.isNull()) {
        qCCritical(pemHttpClient) << "加载私钥失败，私钥为空";
        return false;
    }

    qCInfo(pemHttpClient) << "证书和私钥加载成功";
    return true;
}

QSslConfiguration PemHttpClient::createSslConfiguration()
{
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setLocalCertificate(m_clientCertificate);
    sslConfig.setPrivateKey(m_privateKey);
    
    // 设置TLS版本
    sslConfig.setProtocol(QSsl::TlsV1_2);
    
    // 对于自签名证书或IP地址，跳过主机名验证
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    
    return sslConfig;
}

QString PemHttpClient::getClientCertificate() const
{
    return m_clientCertificate.toPem();
}

QString PemHttpClient::getDeviceUuid() const
{
    // 获取所有网络接口
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    for (const QNetworkInterface &interface : interfaces) {
        // 跳过回环接口
        if (interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }
        
        // 获取接口的硬件地址（MAC地址）
        QString macAddress = interface.hardwareAddress();
        
        // 检查MAC地址是否有效（非空且不是全0）
        if (!macAddress.isEmpty() && macAddress != "00:00:00:00:00:00") {
            // 返回第一个有效的非回环网卡的MAC地址
            return macAddress;
        }
    }
    
    // 如果没有找到有效的MAC地址，尝试从存储中读取UUID
    QString storedUuid = readStoredUuid();
    if (!storedUuid.isEmpty()) {
        return storedUuid;
    }
    
    // 如果存储中也没有UUID，生成一个新的UUID并存储
    QString newUuid = generateDeviceUuid();
    storeUuid(newUuid);
    return newUuid;
}

QString PemHttpClient::generateDeviceUuid() const
{
    // 生成UUID
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString PemHttpClient::encryptString(const QString &input, const QString &key) const
{
    QByteArray inputBytes = input.toUtf8();
    QByteArray keyBytes = key.toUtf8();
    
    QByteArray result;
    for (int i = 0; i < inputBytes.length(); ++i) {
        char encryptedByte = inputBytes[i] ^ keyBytes[i % keyBytes.length()];
        result.append(encryptedByte);
    }
    
    // 转换为Base64以确保安全存储
    return result.toBase64();
}

QString PemHttpClient::decryptString(const QString &input, const QString &key) const
{
    QByteArray encryptedBytes = QByteArray::fromBase64(input.toUtf8());
    QByteArray keyBytes = key.toUtf8();
    
    QByteArray result;
    for (int i = 0; i < encryptedBytes.length(); ++i) {
        char decryptedByte = encryptedBytes[i] ^ keyBytes[i % keyBytes.length()];
        result.append(decryptedByte);
    }
    
    return QString::fromUtf8(result);
}

QString PemHttpClient::readStoredUuid() const
{
    QFile file("device_uuid.dat");
    if (!file.open(QIODevice::ReadOnly)) {
        return QString(); // 文件不存在或无法打开
    }
    
    QString encryptedUuid = QString::fromUtf8(file.readAll());
    file.close();
    
    // 使用固定密钥进行解密（实际项目中应使用更安全的密钥）
    QString key = "moonlight_device_key_2025";
    QString decryptedUuid = decryptString(encryptedUuid, key);
    
    // 验证解密后的UUID格式（检查是否为有效的UUID格式）
    QRegularExpression uuidRegex("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
    if (!decryptedUuid.isEmpty() && (uuidRegex.match(decryptedUuid).hasMatch() || decryptedUuid.length() > 10)) {
        return decryptedUuid;
    }
    
    // 如果解密失败或格式不正确，返回空字符串
    return QString();
}

void PemHttpClient::storeUuid(const QString &uuid) const
{
    QString key = "moonlight_device_key_2025";
    QString encryptedUuid = encryptString(uuid, key);
    
    QFile file("device_uuid.dat");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(encryptedUuid.toUtf8());
        file.close();
    }
}

void PemHttpClient::sendGetRequest(const QString &path, QObject *callbackObject, const QString &callbackMethod)
{
    QUrl url(m_baseUrl + "/" + path.mid(path.startsWith('/') ? 1 : 0));
    qCInfo(pemHttpClient) << "发送GET请求到:" << url.toString();

    QNetworkRequest request(url);
    request.setSslConfiguration(createSslConfiguration());
    request.setRawHeader("User-Agent", "Moonlight Qt Client");
    request.setRawHeader("channelId", "pang_bao"); // 添加渠道ID头

    QNetworkReply *reply = m_networkManager->get(request);

    // 使用lambda处理响应
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, callbackObject, callbackMethod]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qCInfo(pemHttpClient) << "响应内容长度:" << responseData.length() << "字节";

            emit requestFinished(QString::fromUtf8(responseData));
            
            // 如果提供了回调对象和方法，则调用
            if (callbackObject && !callbackMethod.isEmpty()) {
                QMetaObject::invokeMethod(callbackObject, callbackMethod.toLatin1().constData(), 
                                        Q_ARG(QString, QString::fromUtf8(responseData)));
            }
        } else {
            QString errorString = reply->errorString();
            qCCritical(pemHttpClient) << "HTTP请求失败:" << errorString;
            
            emit requestError(errorString);
            
            // 如果提供了回调对象和方法，则调用
            if (callbackObject && !callbackMethod.isEmpty()) {
                QMetaObject::invokeMethod(callbackObject, "onRequestError", 
                                        Q_ARG(QString, errorString));
            }
        }
        
        reply->deleteLater();
    });
}

void PemHttpClient::sendPostRequest(const QString &path, const QString &jsonBody, QObject *callbackObject, const QString &callbackMethod)
{
    QUrl url(m_baseUrl + "/" + path.mid(path.startsWith('/') ? 1 : 0));
    qCInfo(pemHttpClient) << "发送POST请求到:" << url.toString();

    QNetworkRequest request(url);
    request.setSslConfiguration(createSslConfiguration());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "Moonlight Qt Client");
    request.setRawHeader("channelId", "pang_bao"); // 添加渠道ID头

    QNetworkReply *reply = m_networkManager->post(request, jsonBody.toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [this, reply, jsonBody, callbackObject, callbackMethod]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qCInfo(pemHttpClient) << "响应内容长度:" << responseData.length() << "字节";

            emit requestFinished(QString::fromUtf8(responseData));
            
            // 如果提供了回调对象和方法，则调用
            if (callbackObject && !callbackMethod.isEmpty()) {
                QMetaObject::invokeMethod(callbackObject, callbackMethod.toLatin1().constData(), 
                                        Q_ARG(QString, QString::fromUtf8(responseData)));
            }
        } else {
            QString errorString = reply->errorString();
            qCCritical(pemHttpClient) << "HTTP请求失败:" << errorString;
            
            emit requestError(errorString);
            
            // 如果提供了回调对象和方法，则调用
            if (callbackObject && !callbackMethod.isEmpty()) {
                QMetaObject::invokeMethod(callbackObject, "onRequestError", 
                                        Q_ARG(QString, errorString));
            }
        }
        
        reply->deleteLater();
    });
}

// 云游戏客户端相关接口实现
void PemHttpClient::registerDevice(const QString &deviceCode, const QString &model, const QString &type, QObject *callbackObject, const QString &callbackMethod)
{
    QJsonObject requestData;
    requestData["deviceCode"] = deviceCode;
    requestData["model"] = model;
    requestData["type"] = type;
    
    QJsonDocument doc(requestData);
    QString jsonBody = doc.toJson(QJsonDocument::Compact);
    
    sendPostRequest("/business/container/device/register", jsonBody, callbackObject, callbackMethod);
}

void PemHttpClient::getQrcode(const QString &deviceId, QObject *callbackObject, const QString &callbackMethod)
{
    QString path = QString("/business/devices/qrcode?deviceId=%1").arg(deviceId);
    sendGetRequest(path, callbackObject, callbackMethod);
}

void PemHttpClient::queryFreeWindows(const QString &gameId, const QString &userId, const QString &gpuId, QObject *callbackObject, const QString &callbackMethod)
{
    QJsonObject requestData;
    requestData["gameId"] = gameId;
    requestData["gpuId"] = gpuId;
    requestData["token"] = userId;
    
    QJsonDocument doc(requestData);
    QString jsonBody = doc.toJson(QJsonDocument::Compact);
    
    sendPostRequest("/business/cloudGameApp/queryFreeWindows", jsonBody, callbackObject, callbackMethod);
}