import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import QtQuick.Controls.Material 2.2

import ComputerManager 1.0
import AutoUpdateChecker 1.0
import StreamingPreferences 1.0
import SystemProperties 1.0
import SdlGamepadKeyNavigation 1.0
import PemHttpClient 1.0

ApplicationWindow {
    id: window

    // Set by SettingsView to force the back operation to pop all
    // pages except the initial view. This is required when doing
    // a retranslate() because AppView breaks for some reason.
    property bool clearOnBack: false
    property string currentDeviceId: ""
    property string currentQrCodeUrl: ""

    // 设备注册相关属性
    property bool deviceRegistered: false
    property bool pollingActive: false
    property bool showQrCode: false

    // 这个函数运行在创建初始StackView项之前
    function doEarlyInit() {
        // Override the background color to Material 2 colors for Qt 6.5+
        // in order to improve contrast between GFE's placeholder box art
        // and the background of the app grid.
        if (SystemProperties.usesMaterial3Theme) {
            Material.background = "#303030";
        }

        SdlGamepadKeyNavigation.enable();
    }
    function goBack() {
        if (clearOnBack) {
            // Pop all items except the first one
            stackView.pop(null);
            clearOnBack = false;
        } else {
            stackView.pop();
        }
    }
    function navigateTo(url, objectType) {
        var existingItem = stackView.find(function (item, index) {
            return item instanceof objectType;
        });

        if (existingItem !== null) {
            // Pop to the existing item
            stackView.pop(existingItem);
        } else {
            // Create a new item
            stackView.push(url);
        }
    }

    // 获取二维码响应处理函数
    function onGetQrcodeResponse(response) {
        console.log("获取二维码响应: " + response);
        // 同时返回的内容要写入日志
        console.log("完整响应内容: " + response);
        try {
            var responseObj = JSON.parse(response);
            if (responseObj.code === 200) {
                var qrCodeData = responseObj.data;
                var qrCodeValue = qrCodeData.qrCodeUrl;

                // 检查是否是Base64编码的图像数据
                if (qrCodeValue && qrCodeValue.indexOf('data:image') === 0) {
                    // 如果是data URL格式，直接使用
                    currentQrCodeUrl = qrCodeValue;
                } else if (qrCodeValue && qrCodeValue.length > 50) {
                    // 粗略判断是否为Base64字符串
                    // 假设是Base64编码的图像，转换为data URL格式
                    currentQrCodeUrl = "data:image/png;base64," + qrCodeValue;
                } else {
                    // 如果不是Base64编码，假定是普通URL
                    currentQrCodeUrl = qrCodeValue;
                }

                console.log("二维码URL: " + currentQrCodeUrl);
            } else {
                console.log("获取二维码失败: " + responseObj.msg);
            }
        } catch (e) {
            console.log("解析二维码响应失败: " + e);
        }
    }
    function onQueryFreeWindows(response) {
        console.log("查询虚拟机响应: " + response);
        try {
            var responseObj = JSON.parse(response);
            if (responseObj.code === 200) {
                var deviceInfo = responseObj.data;
            } else {
                console.log("查询虚拟机册失败: " + responseObj.msg);
            }
        } catch (e) {
            console.log("解析查询虚拟机响应失败: " + e);
        }
    }

    // 注册设备响应处理函数
    function onRegisterDeviceResponse(response) {
        console.log("注册设备响应: " + response);
        try {
            var responseObj = JSON.parse(response);
            if (responseObj.code === 200) {
                var deviceInfo = responseObj.data;
                currentDeviceId = deviceInfo.deviceId;
                console.log("设备ID: " + currentDeviceId);

                if (deviceInfo.isBind === true) {
                    console.log("设备已绑定，跳过二维码显示");
                    deviceRegistered = true;
                    // 现在可以继续正常流程

                    PemHttpClient.queryFreeWindows(currentDeviceId, deviceInfo.userInfo.userId, "4060", window, "onQueryFreeWindows");
                    showInitialView();
                } else {
                    console.log("设备未绑定，获取二维码");
                    showQrCode = true;

                    if (currentQrCodeUrl === "") {
                        PemHttpClient.getQrcode(currentDeviceId, window, "onGetQrcodeResponse");
                    }
                }
            } else {
                console.log("设备注册失败: " + responseObj.msg);
                // 即使失败也继续显示界面，但可能需要用户手动处理
                // showInitialView()
            }
        } catch (e) {
            console.log("解析注册响应失败: " + e);
            // showInitialView()
        }
    }

    // 显示初始视图
    function showInitialView() {
        // 执行我们的早期初始化，然后推送初始视图到StackView
        doEarlyInit();
        stackView.push(initialView);
    }

    // 设备注册流程 - 现在在Component.onCompleted中调用，确保UI已初始化
    function startDeviceRegistration() {
        console.log("开始设备注册流程");
        var deviceCode = PemHttpClient.getDeviceUuid();
        console.log("设备UUID: " + deviceCode);
        PemHttpClient.registerDevice(deviceCode, "win10", "5", window, "onRegisterDeviceResponse");
    }

    // This configures the maximum width of the singleton attached QML ToolTip. If left unconstrained,
    // it will never insert a line break and just extend on forever.
    ToolTip.toolTip.contentWidth: Math.min(tooltipTextLayoutHelper.width, 400)
    height: 600
    width: 1280

    header: ToolBar {
        id: toolBar

        anchors.bottomMargin: 5
        anchors.topMargin: 5
        height: 60

        Label {
            id: titleLabel

            anchors.fill: parent
            elide: Label.ElideRight
            font.pointSize: 20
            horizontalAlignment: Qt.AlignHCenter
            text: stackView.currentItem ? stackView.currentItem.objectName : ""
            verticalAlignment: Qt.AlignVCenter
            visible: toolBar.width > 700
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 10

            NavigableToolButton {
                iconSource: "qrc:/res/arrow_left.svg"
                // Only make the button visible if the user has navigated somewhere.
                visible: stackView.depth > 1

                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus);
                }
                onClicked: goBack()
            }

            // This label will appear when the window gets too small and
            // we need to ensure the toolbar controls don't collide
            Label {
                id: titleRowLabel

                Layout.fillWidth: true
                elide: Label.ElideRight
                font.pointSize: titleLabel.font.pointSize
                horizontalAlignment: Qt.AlignHCenter

                // We need this label to always be visible so it can occupy
                // the remaining space in the RowLayout. To "hide" it, we
                // just set the text to empty string.
                text: !titleLabel.visible ? (stackView.currentItem ? stackView.currentItem.objectName : "") : ""
                verticalAlignment: Qt.AlignVCenter
            }
            Label {
                id: versionLabel

                font.pointSize: 12
                horizontalAlignment: Qt.AlignRight
                text: qsTr("Version %1").arg(SystemProperties.versionString)
                verticalAlignment: Qt.AlignVCenter
                visible: stackView.currentItem instanceof SettingsView
            }
            NavigableToolButton {
                id: discordButton

                ToolTip.delay: 1000
                ToolTip.text: qsTr("Join our community on Discord")
                ToolTip.timeout: 3000
                ToolTip.visible: hovered
                iconSource: "qrc:/res/discord.svg"
                visible: SystemProperties.hasBrowser && stackView.currentItem instanceof SettingsView

                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus);
                }

                // TODO need to make sure browser is brought to foreground.
                onClicked: Qt.openUrlExternally("https://moonlight-stream.org/discord")
            }
            NavigableToolButton {
                id: addPcButton

                ToolTip.delay: 1000
                ToolTip.text: qsTr("Add PC manually") + (newPcShortcut.nativeText ? (" (" + newPcShortcut.nativeText + ")") : "")
                ToolTip.timeout: 3000
                ToolTip.visible: hovered
                iconSource: "qrc:/res/ic_add_to_queue_white_48px.svg"
                visible: stackView.currentItem instanceof PcView

                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus);
                }
                onClicked: {
                    addPcDialog.open();
                }

                Shortcut {
                    id: newPcShortcut

                    sequence: StandardKey.New

                    onActivated: addPcButton.clicked()
                }
            }
            NavigableToolButton {
                id: updateButton

                property string browserUrl: ""

                function updateAvailable(version, url) {
                    ToolTip.text = qsTr("Update available for Moonlight: Version %1").arg(version);
                    updateButton.browserUrl = url;
                    updateButton.visible = true;
                }

                ToolTip.delay: 1000
                ToolTip.timeout: 3000
                ToolTip.visible: hovered || visible
                iconSource: "qrc:/res/update.svg"

                // Invisible until we get a callback notifying us that
                // an update is available
                visible: false

                Component.onCompleted: {
                    AutoUpdateChecker.onUpdateAvailable.connect(updateAvailable);
                    AutoUpdateChecker.start();
                }
                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus);
                }
                onClicked: {
                    if (SystemProperties.hasBrowser) {
                        Qt.openUrlExternally(browserUrl);
                    }
                }
            }
            NavigableToolButton {
                id: helpButton

                ToolTip.delay: 1000
                ToolTip.text: qsTr("Help") + (helpShortcut.nativeText ? (" (" + helpShortcut.nativeText + ")") : "")
                ToolTip.timeout: 3000
                ToolTip.visible: hovered
                iconSource: "qrc:/res/question_mark.svg"
                visible: SystemProperties.hasBrowser

                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus);
                }

                // TODO need to make sure browser is brought to foreground.
                onClicked: Qt.openUrlExternally("https://github.com/moonlight-stream/moonlight-docs/wiki/Setup-Guide")

                Shortcut {
                    id: helpShortcut

                    sequence: StandardKey.HelpContents

                    onActivated: helpButton.clicked()
                }
            }
            NavigableToolButton {
                ToolTip.delay: 1000
                ToolTip.text: qsTr("Gamepad Mapper")
                ToolTip.timeout: 3000
                ToolTip.visible: hovered
                iconSource: "qrc:/res/ic_videogame_asset_white_48px.svg"
                // TODO: Implement gamepad mapping then unhide this button
                visible: false

                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus);
                }
                onClicked: navigateTo("qrc:/gui/GamepadMapper.qml", GamepadMapper)
            }
            NavigableToolButton {
                id: settingsButton

                ToolTip.delay: 1000
                ToolTip.text: qsTr("Settings") + (settingsShortcut.nativeText ? (" (" + settingsShortcut.nativeText + ")") : "")
                ToolTip.timeout: 3000
                ToolTip.visible: hovered
                iconSource: "qrc:/res/settings.svg"

                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus);
                }
                onClicked: navigateTo("qrc:/gui/SettingsView.qml", SettingsView)

                Shortcut {
                    id: settingsShortcut

                    sequence: StandardKey.Preferences

                    onActivated: settingsButton.clicked()
                }
            }
        }
    }

    Component.onCompleted: {
        // Show the window according to the user's preferences
        if (SystemProperties.hasDesktopEnvironment) {
            if (StreamingPreferences.uiDisplayMode == StreamingPreferences.UI_MAXIMIZED) {
                window.showMaximized();
            } else if (StreamingPreferences.uiDisplayMode == StreamingPreferences.UI_FULLSCREEN) {
                window.showFullScreen();
            } else {
                window.show();
            }
        } else {
            window.showFullScreen();
        }

        // Display any modal dialogs for configuration warnings
        if (SystemProperties.isWow64) {
            wow64Dialog.open();
        } else if (!SystemProperties.hasHardwareAcceleration && StreamingPreferences.videoDecoderSelection !== StreamingPreferences.VDS_FORCE_SOFTWARE) {
            if (SystemProperties.isRunningXWayland) {
                xWaylandDialog.open();
            } else {
                noHwDecoderDialog.open();
            }
        }

        if (SystemProperties.unmappedGamepads) {
            unmappedGamepadDialog.unmappedGamepads = SystemProperties.unmappedGamepads;
            unmappedGamepadDialog.open();
        }

        // 在UI组件完全初始化后再开始设备注册流程，以避免触发SDLGamepadKeyNavigation断言
        startDeviceRegistration();
    }
    onActiveChanged: {
        if (active) {
            // Stop the inactivity timer
            inactivityTimer.stop();

            // Restart polling if it was stopped
            if (!pollingActive) {
                ComputerManager.startPolling();
                pollingActive = true;
            }
        } else {
            // Start the inactivity timer to stop polling
            // if focus does not return within a few minutes.
            inactivityTimer.restart();
        }

        // Poll for gamepad input only when the window is in focus
        SdlGamepadKeyNavigation.notifyWindowFocus(visible && active);
    }
    onVisibleChanged: {
        // When we become invisible while streaming is going on,
        // stop polling immediately.
        if (!visible) {
            inactivityTimer.stop();

            if (pollingActive) {
                ComputerManager.stopPollingAsync();
                pollingActive = false;
            }
        } else if (active) {
            // When we become visible and active again, start polling
            inactivityTimer.stop();

            // Restart polling if it was stopped
            if (!pollingActive) {
                ComputerManager.startPolling();
                pollingActive = true;
            }
        }

        // Poll for gamepad input only when the window is in focus
        SdlGamepadKeyNavigation.notifyWindowFocus(visible && active);
    }

    // 检查设备绑定状态的定时器
    Timer {
        id: checkBindingTimer

        interval: 5000 // 每5秒检查一次
        repeat: true
        running: showQrCode && !deviceRegistered

        onTriggered: {
            if (showQrCode && !deviceRegistered) {
                console.log("检查设备绑定状态");
                var deviceCode = PemHttpClient.getDeviceUuid();
                PemHttpClient.registerDevice(deviceCode, "win10", "5", window, "onRegisterDeviceResponse");
            }
        }
    }

    // It would be better to use TextMetrics here, but it always lays out
    // the text slightly more compactly than real Text does in ToolTip,
    // causing unexpected line breaks to be inserted
    Text {
        id: tooltipTextLayoutHelper

        font: ToolTip.toolTip.font
        text: ToolTip.toolTip.text
        visible: false
    }
    StackView {
        id: stackView

        anchors.fill: parent
        focus: true

        Component.onCompleted: {
            // 如果设备已经注册，直接显示初始视图
            if (deviceRegistered) {
                showInitialView();
            }
        }
        Keys.onBackPressed: {
            if (depth > 1) {
                goBack();
            } else {
                quitConfirmationDialog.open();
            }
        }
        Keys.onEscapePressed: {
            if (depth > 1) {
                goBack();
            } else {
                quitConfirmationDialog.open();
            }
        }

        // This is a keypress we've reserved for letting the
        // SdlGamepadKeyNavigation object tell us to show settings
        // when Menu is consumed by a focused control.
        Keys.onHangupPressed: {
            settingsButton.clicked();
        }
        Keys.onMenuPressed: {
            settingsButton.clicked();
        }
        onCurrentItemChanged: {
            // Ensure focus travels to the next view when going back
            if (currentItem) {
                currentItem.forceActiveFocus();
            }
        }
    }

    // 二维码显示覆盖层
    Rectangle {
        id: qrCodeOverlay

        anchors.fill: parent
        color: "#80000000" // 半透明黑色背景
        visible: showQrCode && !deviceRegistered

        Column {
            anchors.centerIn: parent
            spacing: 20

            Text {
                color: "white"
                font.pointSize: 18
                horizontalAlignment: Text.AlignHCenter
                text: "请扫描二维码绑定设备"
            }

            // 二维码图片显示（使用Base64 URL）
            Image {
                id: qrCodeImage

                fillMode: Image.PreserveAspectFit
                height: 200
                source: currentQrCodeUrl ? currentQrCodeUrl : ""
                visible: currentQrCodeUrl !== ""
                width: 200

                onSourceChanged: {
                    console.log("二维码图片源已更新: " + source);
                }
            }

            // 显示加载状态
            Text {
                id: loadingText

                color: "white"
                font.pointSize: 14
                text: "等待设备绑定..."
                visible: currentQrCodeUrl === ""
            }
            Button {
                text: "刷新二维码"

                onClicked: {
                    if (currentDeviceId) {
                        PemHttpClient.getQrcode(currentDeviceId, window, "onGetQrcodeResponse");
                    }
                }
            }
        }
    }

    // This timer keeps us polling for 5 minutes of inactivity
    // to allow the user to work with Moonlight on a second display
    // while dealing with configuration issues. This will ensure
    // machines come online even if the input focus isn't on Moonlight.
    Timer {
        id: inactivityTimer

        interval: 5 * 60000

        onTriggered: {
            if (!active && pollingActive) {
                ComputerManager.stopPollingAsync();
                pollingActive = false;
            }
        }
    }
    ErrorMessageDialog {
        id: noHwDecoderDialog

        helpText: qsTr("Click the Help button for more information on solving this problem.")
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Fixing-Hardware-Decoding-Problems"
        text: qsTr("No functioning hardware accelerated video decoder was detected by Moonlight. " + "Your streaming performance may be severely degraded in this configuration.")
    }
    ErrorMessageDialog {
        id: xWaylandDialog

        helpText: qsTr("Click the Help button for more information.")
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Fixing-Hardware-Decoding-Problems"
        text: qsTr("Hardware acceleration doesn't work on XWayland. Continuing on XWayland may result in poor streaming performance. " + "Try running with QT_QPA_PLATFORM=wayland or switch to X11.")
    }
    NavigableMessageDialog {
        id: wow64Dialog

        standardButtons: Dialog.Ok | Dialog.Cancel
        text: qsTr("This version of Moonlight isn't optimized for your PC. Please download the '%1' version of Moonlight for the best streaming performance.").arg(SystemProperties.friendlyNativeArchName)

        onAccepted: {
            Qt.openUrlExternally("https://github.com/moonlight-stream/moonlight-qt/releases");
        }
    }
    ErrorMessageDialog {
        id: unmappedGamepadDialog

        property string unmappedGamepads: ""

        helpText: qsTr("Click the Help button for information on how to map your gamepads.")
        helpTextSeparator: "\n\n"
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Gamepad-Mapping"
        text: qsTr("Moonlight detected gamepads without a mapping:") + "\n" + unmappedGamepads
    }

    // This dialog appears when quitting via keyboard or gamepad button
    NavigableMessageDialog {
        id: quitConfirmationDialog

        standardButtons: Dialog.Yes | Dialog.No
        text: qsTr("Are you sure you want to quit?")

        // For keyboard/gamepad navigation
        onAccepted: Qt.quit()
    }

    // HACK: This belongs in StreamSegue but keeping a dialog around after the parent
    // dies can trigger bugs in Qt 5.12 that cause the app to crash. For now, we will
    // host this dialog in a QML component that is never destroyed.
    //
    // To repro: Start a stream, cut the network connection to trigger the "Connection
    // terminated" dialog, wait until the app grid times out back to the PC grid, then
    // try to dismiss the dialog.
    ErrorMessageDialog {
        id: streamSegueErrorDialog

        property bool quitAfter: false

        onClosed: {
            if (quitAfter) {
                quitAfter = false;

                // StreamSegue assumes its dialog will be re-created each time we
                // start streaming, so fake it by wiping out the text each time.
                text = "";
            }
        }
    }
    NavigableDialog {
        id: addPcDialog

        property string label: qsTr("Enter the IP address of your host PC:")

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            if (editText.text) {
                ComputerManager.addNewHostManually(editText.text.trim());
            }
        }
        onClosed: {
            editText.clear();
        }
        onOpened: {
            // Force keyboard focus on the textbox so keyboard navigation works
            editText.forceActiveFocus();
        }

        ColumnLayout {
            Label {
                font.bold: true
                text: addPcDialog.label
            }
            TextField {
                id: editText

                Layout.fillWidth: true
                focus: true

                Keys.onEnterPressed: {
                    addPcDialog.accept();
                }
                Keys.onReturnPressed: {
                    addPcDialog.accept();
                }
            }
        }
    }
}