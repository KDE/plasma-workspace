/********************************************************************
 This file is part of the KDE project.

Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kscreenlocker 1.0
import "../components"

Image {
    id: root
    property bool debug: false
    property bool shutdownSupported: true
    property string notification
    property UserSelect userSelect: null
    signal shutdown()
    signal clearPassword()

    source: "../components/artwork/background.png"

    Connections {
        target: authenticator
        onFailed: {
            root.notification = i18nd("plasma_lookandfeel_org.kde.lookandfeel","Unlocking failed");
            root.clearPassword()
        }
        onGraceLockedChanged: {
            if (!authenticator.graceLocked) {
                root.notification = "";
                root.clearPassword();
            }
        }
        onMessage: function(text) {
            root.notification = text;
        }
        onError: function(text) {
            root.notification = text;
        }
    }
    Sessions {
        id: sessions
    }

    StackView {
        id: stackView
        height: units.largeSpacing*14
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            right: parent.right
        }

        initialItem: BreezeBlock {
            id: block
            main: UserSelect {
                id: usersSelection

                onVisibleChanged: {
                    if(visible) {
                        currentIndex = 0;
                    }
                }
                Component.onCompleted: root.userSelect = usersSelection

                notification: root.notification

                model: ListModel {
                    id: users

                    Component.onCompleted: {
                        users.append({  "name": kscreenlocker_userName,
                                        "realName": kscreenlocker_userName,
                                        "icon": kscreenlocker_userImage,
                                        "showPassword": true,
                                        "ButtonLabel": i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Unlock"),
                                        "ButtonAction": "unlock"
                        })
                        if(sessions.startNewSessionSupported) {
                            users.append({  "realName": i18nd("plasma_lookandfeel_org.kde.lookandfeel", "New Session"),
                                            "icon": "system-log-out", //TODO Need an icon for new session
                                            "showPassword": false,
                                            "ButtonLabel": i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Create Session"),
                                            "ButtonAction": "newSession"
                            })
                        }
                        if(sessions.switchUserSupported) {
                            users.append({  "realName": i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Change Session"),
                                            "icon": "system-switch-user",
                                            "showPassword": false,
                                            "ButtonLabel": i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Change Session..."),
                                            "ButtonAction": "changeSession"
                            })
                        }
                    }
                }
            }

            controls: Item {
                height: childrenRect.height
                Layout.fillWidth: true
                function unlockFunction() {
                    authenticator.tryUnlock(passwordInput.text);
                }

                RowLayout {

                    anchors.horizontalCenter: parent.horizontalCenter
                    PlasmaComponents.TextField {
                        id: passwordInput
                        placeholderText: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Password")
                        echoMode: TextInput.Password
                        enabled: !authenticator.graceLocked
                        onAccepted: unlockFunction()
                        focus: true
                        visible: block.mainItem.model.get(block.mainItem.selectedIndex)["showPassword"]

                        Keys.onLeftPressed: {
                            if (text == "") {
                                root.userSelect.decrementCurrentIndex();
                            } else {
                                event.accepted = false;
                            }
                        }
                        Keys.onRightPressed: {
                            if (text == "") {
                                root.userSelect.incrementCurrentIndex();
                            } else {
                                event.accepted = false;
                            }
                        }
                    }

                    PlasmaComponents.Button {
                        Layout.minimumWidth: passwordInput.width
                        text: block.mainItem.model.get(block.mainItem.selectedIndex)["ButtonLabel"]
                        enabled: !authenticator.graceLocked
                        onClicked: switch(block.mainItem.model.get(block.mainItem.selectedIndex)["ButtonAction"]) {
                            case "unlock":
                                unlockFunction();
                                break;
                            case "newSession":
                                sessions.createNewSession();
                                break;
                            case "changeSession":
                                stackView.push(changeSessionComponent)
                                break;
                        }
                        Keys.onLeftPressed: {
                            root.userSelect.decrementCurrentIndex();
                        }
                        Keys.onRightPressed: {
                            root.userSelect.incrementCurrentIndex();
                        }
                    }

                    Connections {
                        target: root
                        onClearPassword: {
                            passwordInput.selectAll();
                            passwordInput.forceActiveFocus();
                        }
                    }
                }

                LogoutOptions {
                    id: logoutOptions
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    canReboot: false
                    canLogout: false
                    canShutdown: root.shutdownSupported
                    mode: ""
                    exclusive: false
                    onModeChanged: {
                        if(mode != "")
                            stackView.push(logoutScreenComponent, {"mode": logoutOptions.mode })
                    }
                    onVisibleChanged: if(visible) {
                        mode = ""
                    }
                }

                Component {
                    id: logoutScreenComponent
                    LogoutScreen {
                        canReboot: logoutOptions.canReboot
                        canLogout: logoutOptions.canLogout
                        canShutdown: logoutOptions.canShutdown
                        onCancel: stackView.pop()

                        onShutdownRequested: {
                            root.shutdown()
                        }
                    }
                }

                Component {
                    id: changeSessionComponent
                    BreezeBlock {
                        id: selectSessionBlock

                        Action {
                            onTriggered: stackView.pop()
                            shortcut: "Escape"
                        }

                        main: UserSelect {
                            id: sessionSelect

                            model: sessions.model
                            delegate: UserDelegate {
                                name: i18nd("plasma_lookandfeel_org.kde.lookandfeel","%1 (%2)", model.session, model.location)
                                userName: model.session
                                iconSource: "user-identity"
                                width: ListView.view.userItemWidth
                                height: ListView.view.userItemHeight
                                faceSize: ListView.view.userFaceSize

                                onClicked: {
                                    ListView.view.currentIndex = index;
                                    ListView.view.forceActiveFocus();
                                }
                            }
                        }

                        controls: Item {
                            height: childrenRect.height
                            RowLayout {
                                anchors.centerIn: parent
                                PlasmaComponents.Button {
                                    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Cancel")
                                    onClicked: stackView.pop()
                                }
                                PlasmaComponents.Button {
                                    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Change Session")
                                    onClicked: sessions.activateSession(selectSessionBlock.mainItem.selectedIndex)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
