/*
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1 as Controls

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

import SddmComponents 2.0

import "./components"

Image {
    id: root
    width: 1000
    height: 1000

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    //we made some reverts, but I want to keep some i18n messages around so we don't lose the translations if/when we restore this
    property string unused1: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Select User")
    property string unused2: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Search for User")

    Repeater {
        model: screenModel
        Background {
            x: geometry.x; y: geometry.y; width: geometry.width; height:geometry.height
            source: config.background
            fillMode: Image.PreserveAspectCrop
            onStatusChanged: {
                if (status == Image.Error && source != config.defaultBackground) {
                    source = config.defaultBackground
                }
            }
        }
    }

    property bool debug: false

    Rectangle {
        id: debug3
        color: "green"
        visible: debug
        width: 3
        height: parent.height
        anchors.horizontalCenter: root.horizontalCenter
    }

    Controls.StackView {
        id: stackView

        //Display the loginpromt only in the primary screen
        readonly property rect geometry: screenModel.geometry(screenModel.primary)
        width: geometry.width
        x: geometry.x
        height: units.largeSpacing*14
        //Display the BreezeBlock in the middle of each screen
        y: geometry.y + (geometry.height / 2) - (height / 2)

        initialItem: BreezeBlock {
            id: loginPrompt

            //Enable clipping whilst animating, otherwise the items would be shifted to other screens in multiscreen setups
            //As there are only 2 items (loginPrompt and logoutScreenComponent), it's sufficient to do it only in this component
            //Remember to enable clipping whilst animating when creating additional items for the StackView!
            Controls.Stack.onStatusChanged: {
                if(Controls.Stack.status === Controls.Stack.Activating || Controls.Stack.status === Controls.Stack.Deactivating){
                    stackView.clip = true;
                }else if(Controls.Stack.status === Controls.Stack.Active || Controls.Stack.status === Controls.Stack.Inactive){
                    stackView.clip = false;
                }
            }

            main: UserSelect {
                model: userModel
                selectedIndex: userModel.lastIndex

                //Resets the "Login Failed" notification after 3 seconds
                Timer {
                    id: notificationResetTimer
                    interval: 3000
                    onTriggered: notification = ""
                }

                Connections {
                    target: sddm
                    onLoginFailed: {
                        notificationResetTimer.restart()
                        notification = i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Login Failed")
                    }
                }

            }

            controls: Item {
                height: childrenRect.height

                property alias password: passwordInput.text
                property alias sessionIndex: sessionCombo.currentIndex
                property alias buttonEnabled: loginButton.enabled
                property alias pwFieldEnabled: passwordInput.enabled

                ColumnLayout {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 0
                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter

                        PlasmaComponents.Button {
                            id: kbdLayoutButton
                            implicitWidth: minimumWidth
                            text: keyboard.layouts[keyboard.currentLayout].shortName
                            visible: keyboard.layouts.length > 1

                            onClicked: {
                                var idx = (keyboard.currentLayout + 1) % keyboard.layouts.length;
                                keyboard.currentLayout = idx;
                            }

                            KeyNavigation.tab: sessionCombo
                        }

                        PlasmaComponents.TextField {
                            id: passwordInput
                            placeholderText: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Password")
                            echoMode: TextInput.Password
                            onAccepted: loginPrompt.startLogin()
                            focus: true

                            //focus works in qmlscene
                            //but this seems to be needed when loaded from SDDM
                            //I don't understand why, but we have seen this before in the old lock screen
                            Timer {
                                interval: 200
                                running: true
                                onTriggered: passwordInput.forceActiveFocus()
                            }
                            //end hack

                            Keys.onEscapePressed: {
                                loginPrompt.mainItem.forceActiveFocus();
                            }

                            //if empty and left or right is pressed change selection in user switch
                            //this cannot be in keys.onLeftPressed as then it doesn't reach the password box
                            Keys.onPressed: {
                                if (event.key == Qt.Key_Left && !text) {
                                    loginPrompt.mainItem.decrementCurrentIndex();
                                    event.accepted = true
                                }
                                if (event.key == Qt.Key_Right && !text) {
                                    loginPrompt.mainItem.incrementCurrentIndex();
                                    event.accepted = true
                                }
                            }

                            KeyNavigation.backtab: loginPrompt.mainItem
                        }

                        PlasmaComponents.Button {
                            id: loginButton
                            //this keeps the buttons the same width and thus line up evenly around the centre
                            Layout.minimumWidth: passwordInput.width
                            text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Login")
                            onClicked: loginPrompt.startLogin();

                            KeyNavigation.tab: kbdLayoutButton
                        }
                    }

                    BreezeLabel {
                        id: capsLockWarning
                        text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Caps Lock is on")
                        visible: keystateSource.data["Caps Lock"]["Locked"]

                        anchors.horizontalCenter: parent.horizontalCenter
                        font.weight: Font.Bold

                        PlasmaCore.DataSource {
                            id: keystateSource
                            engine: "keystate"
                            connectedSources: "Caps Lock"
                        }
                    }
                }

                PlasmaComponents.ComboBox {
                    id: sessionCombo
                    model: sessionModel
                    currentIndex: sessionModel.lastIndex

                    width: 200
                    textRole: "name"

                    anchors.left: parent.left
                }

                LogoutOptions {
                    mode: ""
                    canShutdown: true
                    canReboot: true
                    canLogout: false
                    exclusive: false

                    anchors {
                        right: parent.right
                    }

                    onModeChanged: {
                        if (mode) {
                            stackView.push(logoutScreenComponent, {"mode": mode})
                        }
                    }
                    onVisibleChanged: if(visible) {
                                          mode = ""
                                      }
                }

                Connections {
                    target: sddm
                    onLoginFailed: {
                        //Re-enable button and textfield
                        passwordInput.enabled = true
                        passwordInput.selectAll()
                        passwordInput.forceActiveFocus()
                        loginButton.enabled = true;
                    }
                }

            }

            function startLogin () {
                //Disable button and textfield while password check is running
                controlsItem.pwFieldEnabled = false;
                controlsItem.buttonEnabled = false;
                //Clear notification in case the notificationResetTimer hasn't expired yet
                mainItem.notification = ""
                sddm.login(mainItem.selectedUser, controlsItem.password, controlsItem.sessionIndex)
            }

            Component {
                id: logoutScreenComponent
                LogoutScreen {
                    onCancel: {
                        stackView.pop()
                    }

                    onShutdownRequested: {
                        sddm.powerOff()
                    }

                    onRebootRequested: {
                        sddm.reboot()
                    }
                }
            }
        }

    }
}
