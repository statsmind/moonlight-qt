import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Dialog {
    // We should use Overlay.overlay here but that's not available in Qt 5.9 :(
    parent: ApplicationWindow.contentItem

    // 居中显示对话框
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)

    // 不可取消的模态对话框
    modal: true
    closePolicy: Popup.NoAutoClose
    
    // 设置对话框的最小宽度
    width: Math.max(400, implicitWidth)
    height: implicitHeight

    property bool loginInProgress: false

    title: qsTr("用户登录")

    onOpened: {
        // 默认选择手机号+密码登录
        phoneTabButton.checked = true
        phoneField.forceActiveFocus()
    }

    onAboutToHide: {
        // We must force focus back to the last item for platforms without
        // support for more than one active window like Steam Link. If
        // we don't, gamepad and keyboard navigation will break after a
        // dialog appears.
        if (typeof stackView !== 'undefined') {
            stackView.forceActiveFocus()
        }
    }

    ColumnLayout {
        id: mainLayout
        width: parent.width

        TabBar {
            id: loginTypeTabBar
            Layout.fillWidth: true

            TabButton {
                id: phoneTabButton
                text: qsTr("手机号登录")
            }

            TabButton {
                id: verificationCodeTabButton
                text: qsTr("验证码登录")
            }
        }

        StackLayout {
            id: loginMethodStack
            Layout.fillWidth: true
            currentIndex: loginTypeTabBar.currentIndex

            // 手机号+密码登录
            ColumnLayout {
                id: phoneLoginLayout

                Label {
                    text: qsTr("手机号")
                }

                TextField {
                    id: phoneField
                    Layout.fillWidth: true
                    placeholderText: qsTr("请输入手机号")
                    inputMethodHints: Qt.ImhDialableCharactersOnly

                    Keys.onReturnPressed: {
                        if (passwordField.text.length > 0) {
                            loginButton.clicked()
                        } else {
                            passwordField.forceActiveFocus()
                        }
                    }

                    Keys.onEnterPressed: {
                        if (passwordField.text.length > 0) {
                            loginButton.clicked()
                        } else {
                            passwordField.forceActiveFocus()
                        }
                    }
                }

                Label {
                    text: qsTr("密码")
                }

                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: qsTr("请输入密码")
                    echoMode: TextInput.Password

                    Keys.onReturnPressed: {
                        if (phoneField.text.length > 0) {
                            loginButton.clicked()
                        }
                    }

                    Keys.onEnterPressed: {
                        if (phoneField.text.length > 0) {
                            loginButton.clicked()
                        }
                    }
                }
            }

            // 手机号+验证码登录
            ColumnLayout {
                id: verificationCodeLoginLayout

                Label {
                    text: qsTr("手机号")
                }

                TextField {
                    id: phoneField2
                    Layout.fillWidth: true
                    placeholderText: qsTr("请输入手机号")
                    inputMethodHints: Qt.ImhDialableCharactersOnly

                    Keys.onReturnPressed: {
                        if (verificationCodeField.text.length > 0) {
                            loginButton.clicked()
                        } else {
                            verificationCodeField.forceActiveFocus()
                        }
                    }

                    Keys.onEnterPressed: {
                        if (verificationCodeField.text.length > 0) {
                            loginButton.clicked()
                        } else {
                            verificationCodeField.forceActiveFocus()
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("验证码")
                    }

                    Button {
                        id: sendCodeButton
                        text: qsTr("发送验证码")
                        enabled: phoneField2.text.length > 0 && !loginInProgress

                        onClicked: {
                            // 这里应该发送验证码的逻辑
                            // 暂时用一个简单的计时器模拟
                            if (phoneField2.text.length > 0) {
                                sendCodeButton.enabled = false
                                var count = 60
                                var timer = setInterval(function() {
                                    sendCodeButton.text = qsTr("%1秒后重发").arg(count)
                                    count--
                                    if (count <= 0) {
                                        clearInterval(timer)
                                        sendCodeButton.text = qsTr("发送验证码")
                                        sendCodeButton.enabled = true
                                    }
                                }, 1000)
                            }
                        }
                    }
                }

                TextField {
                    id: verificationCodeField
                    Layout.fillWidth: true
                    placeholderText: qsTr("请输入验证码")
                    inputMethodHints: Qt.ImhDialableCharactersOnly

                    Keys.onReturnPressed: {
                        if (phoneField2.text.length > 0) {
                            loginButton.clicked()
                        }
                    }

                    Keys.onEnterPressed: {
                        if (phoneField2.text.length > 0) {
                            loginButton.clicked()
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.topMargin: 10
            Layout.fillWidth: true

            BusyIndicator {
                id: loginProgressIndicator
                visible: loginInProgress
                running: loginInProgress
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                id: cancelButton
                text: qsTr("取消")
                enabled: !loginInProgress

                // 根据需求，这个对话框应该是不可取消的
                visible: false

                onClicked: {
                    // close()
                }
            }

            Button {
                id: loginButton
                text: qsTr("登录")
                enabled: !loginInProgress && isInputValid()

                function isInputValid() {
                    if (loginTypeTabBar.currentIndex === 0) {
                        // 手机号+密码登录
                        return phoneField.text.length > 0 && passwordField.text.length > 0
                    } else {
                        // 验证码登录
                        return phoneField2.text.length > 0 && verificationCodeField.text.length > 0
                    }
                }

                onClicked: {
                    loginInProgress = true
                    // 这里应该添加实际的登录逻辑
                    // 模拟登录过程
                    loginTimer.start()
                }
            }
        }
    }

    Timer {
        id: loginTimer
        interval: 2000

        onTriggered: {
            loginInProgress = false
            // 模拟登录成功
            close()
        }
    }

    function isInputValid() {
        return loginButton.isInputValid()
    }
}