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

import QtQuick 2.5
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

import org.kde.plasma.private.sessions 2.0
import "../components"

PlasmaCore.ColorScope {
    id: lockScreenRoot

    colorGroup: PlasmaCore.Theme.ComplementaryColorGroup

    Connections {
        target: authenticator
        onFailed: {
            root.notification = i18nd("plasma_lookandfeel_org.kde.lookandfeel","Unlocking failed");
        }
        onGraceLockedChanged: {
            if (!authenticator.graceLocked) {
                root.notification = "";
                root.clearPassword();
            }
        }
        onMessage: {
            root.notification = msg;
        }
        onError: {
            root.notification = err;
        }
    }

    SessionsModel {
        id: sessionsModel
        showNewSessionEntry: true
    }

    PlasmaCore.DataSource {
        id: keystateSource
        engine: "keystate"
        connectedSources: "Caps Lock"
    }

    Loader {
        id: changeSessionComponent
        active: false
        source: "ChangeSession.qml"
        visible: false
    }

    Clock {
        anchors.bottom: parent.verticalCenter
        anchors.bottomMargin: units.gridUnit * 13
        anchors.horizontalCenter: parent.horizontalCenter
    }

    ListModel {
        id: users

        Component.onCompleted: {
            users.append({name: kscreenlocker_userName,
                            realName: kscreenlocker_userName,
                            icon: kscreenlocker_userImage,

            })
            if (sessionsModel.canStartNewSession) {
                users.append({realName: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "New Session"),
                                name: "__new_session",
                                iconName: "list-add"
                })
            }
        }
    }

    StackView {
        id: mainStack
        anchors.fill: parent
        focus: true //StackView is an implicit focus scope, so we need to give this focus so the item inside will have it
        initialItem: MainBlock {
            userListModel: users
            notificationMessage: {
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

            onNewSession: {
                sessionsModel.startNewSession(false);
            }

            onLoginRequest: {
                root.notification = ""
                authenticator.tryUnlock(password)
            }

            actionItems: [
                ActionButton {
                    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Switch User")
                    iconSource: "system-switch-user"
                    onClicked: mainStack.push(switchSessionPage)
                    visible: sessionsModel.count > 1 && sessionsModel.canSwitchUser
                }
            ]
        }
    }

    Component {
        id: switchSessionPage
        SessionManagementScreen {
            userListModel: sessionsModel

            PlasmaComponents.Button {
                Layout.fillWidth: true
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Switch Session")
                onClicked: {
                    sessionsModel.switchUser(userListCurrentModelData.vtNumber)
                    mainStack.pop()
                }
            }

            actionItems: [
                ActionButton {
                    iconSource: "go-previous"
                    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Back")
                    onClicked: mainStack.pop()
                }
            ]
        }
    }

    Loader {
        active: root.viewVisible
        source: "LockOsd.qml"
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
    }

    RowLayout {
        id: footer
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
            margins: units.smallSpacing
        }

        KeyboardLayoutButton {
        }

        Item {
            Layout.fillWidth: true
        }

        Battery {}
    }


    Component.onCompleted: {
        // version support checks
        if (root.interfaceVersion < 1) {
            // ksmserver of 5.4, with greeter of 5.5
            root.viewVisible = true;
        }
    }
}
