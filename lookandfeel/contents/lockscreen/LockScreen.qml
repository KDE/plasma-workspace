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
import "../components"

Image {
    id: root
    property bool debug: false
    property bool shutdownSupported: true
    property string notification //TODO: need to figure out where this goes
    signal shutdown()
    signal clearPassword()

    source: "../components/artwork/background.png"

    Connections {
        target: authenticator
        onFailed: {
            root.notification = i18n("Unlocking failed");
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
                model: ListModel {
                    id: users

                    Component.onCompleted: {
                        //TODO: user switching is not yet supported
                        users.append({  "name": kscreenlocker_userName,
                                        "realName": kscreenlocker_userName,
                                        "icon": kscreenlocker_userImage})
                    }
                }
            }

            controls: Item {
                height: childrenRect.height
                Layout.fillWidth: true

                RowLayout {
                    anchors.horizontalCenter: parent.horizontalCenter
                    PlasmaComponents.TextField {
                        id: passwordInput
                        placeholderText: i18n("Password")
                        echoMode: TextInput.Password
                        enabled: !authenticator.graceLocked
                        onAccepted: authenticator.tryUnlock(passwordInput.text)
                        focus: true
                    }

                    PlasmaComponents.Button {
                        Layout.minimumWidth: passwordInput.width
                        text: i18n("Unlock")
                        enabled: !authenticator.graceLocked
                        onClicked: authenticator.tryUnlock(passwordInput.text)
                    }

                    Connections {
                        target: root
                        onClearPassword: {
                            passwordInput.selectAll();
                            passwordInput.focus = true;
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
            }
        }
    }
}
