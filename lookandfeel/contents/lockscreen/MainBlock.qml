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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.workspace.keyboardlayout 1.0
import "../components"

BreezeBlock {
    id: block
    main: UserSelect {
        id: usersSelection

        onVisibleChanged: {
            if(visible) {
                selectedIndex = 0;
            }
        }
        Component.onCompleted: root.userSelect = usersSelection

        notification: {
            var text = ""
            if (keystateSource.data["Caps Lock"]["Locked"]) {
                text += i18nd("plasma_lookandfeel_org.kde.lookandfeel","Caps Lock is on")
                if (root.notification) {
                    text += " • "
                }
            }
            text += root.notification
            return text
        }

        model: ListModel {
            id: users

            Component.onCompleted: {
                users.append({name: kscreenlocker_userName,
                                realName: kscreenlocker_userName,
                                icon: kscreenlocker_userImage,
                                showPassword: true,
                                ButtonLabel: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Unlock"),
                                ButtonAction: "unlock"
                })
                if (sessionsModel.canStartNewSession) {
                    users.append({realName: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "New Session"),
                                    icon: "system-log-out", //TODO Need an icon for new session
                                    ButtonLabel: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Create Session"),
                                    ButtonAction: "newSession"
                    })
                }
                if (sessionsModel.canSwitchUser && sessionsModel.count > 0) {
                    users.append({realName: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Change Session"),
                                    icon: "system-switch-user",
                                    ButtonLabel: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Change Session..."),
                                    ButtonAction: "changeSession"
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

        ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            RowLayout {
                anchors.horizontalCenter: parent.horizontalCenter

                KeyboardLayoutButton {
                    id: kbdLayoutButton
                    hidden: !passwordInput.visible
                    KeyNavigation.tab: block.mainItem
                }

                PlasmaComponents.TextField {
                    id: passwordInput
                    placeholderText: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Password")
                    echoMode: TextInput.Password
                    enabled: !authenticator.graceLocked
                    onAccepted: actionButton.clicked(null)
                    focus: true
                    //HACK: Similar hack is needed in sddm loginscreen
                    //TODO: investigate
                    Timer {
                        interval: 200
                        running: true
                        repeat: false
                        onTriggered: passwordInput.forceActiveFocus()
                    }
                    visible: block.mainItem.model.get(block.mainItem.selectedIndex) ? !!block.mainItem.model.get(block.mainItem.selectedIndex).showPassword : false
                    onVisibleChanged: {
                        if (visible) {
                            forceActiveFocus();
                        }
                        text = "";
                    }
                    onTextChanged: {
                        if (text == "") {
                            clearTimer.stop();
                        } else {
                            clearTimer.restart();
                        }
                    }

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
                    Timer {
                        id: clearTimer
                        interval: 30000
                        repeat: false
                        onTriggered: {
                            passwordInput.text = "";
                        }
                    }
                    KeyNavigation.backtab: block.mainItem
                }

                PlasmaComponents.Button {
                    id: actionButton
                    Layout.minimumWidth: passwordInput.width
                    text: block.mainItem.model.get(block.mainItem.selectedIndex) ? block.mainItem.model.get(block.mainItem.selectedIndex).ButtonLabel : ""
                    enabled: !authenticator.graceLocked
                    onClicked: switch(block.mainItem.model.get(block.mainItem.selectedIndex)["ButtonAction"]) {
                        case "unlock":
                            unlockFunction();
                            break;
                        case "newSession":
                            // false means don't lock, we're the lock screen
                            sessionsModel.startNewSession(false);
                            break;
                        case "changeSession":
                            changeSessionComponent.active = true
                            stackView.push(changeSessionComponent.item)
                            break;
                    }
                    KeyNavigation.tab: kbdLayoutButton
                }

                Connections {
                    target: root
                    onClearPassword: {
                        passwordInput.selectAll();
                        passwordInput.forceActiveFocus();
                    }
                }
                Keys.onLeftPressed: {
                    root.userSelect.decrementCurrentIndex();
                }
                Keys.onRightPressed: {
                    root.userSelect.incrementCurrentIndex();
                }
            }
        }
    }
}
