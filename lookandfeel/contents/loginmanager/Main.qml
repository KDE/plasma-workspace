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

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1 as Controls

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

import "./components"

Image {
    id: root
    width: 1000
    height: 1000

    source: "components/artwork/background.png"
    smooth: true
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

        height: units.largeSpacing*14
        anchors.centerIn: parent

        initialItem: BreezeBlock {
            main: UserSelect {
                id: usersSelection
                model: userModel
            }

            controls: Item {
                height: childrenRect.height

                PlasmaComponents.ComboBox {
                    id: sessionCombo
                    model: sessionModel
                    currentIndex: sessionModel.lastIndex
                    anchors.left: parent.left

                    width: 200

                    textRole: "name"
                }

                RowLayout {
                    anchors.horizontalCenter: parent.horizontalCenter
                    PlasmaComponents.TextField {
                        id: passwordInput
                        placeholderText: i18n("Password")
                        echoMode: TextInput.Password
                        onAccepted: startLogin()
                        focus: true
                    }

                    PlasmaComponents.Button {
                        Layout.minimumWidth: passwordInput.width
                        text: i18n("Login")
                        onClicked: startLogin();
                    }
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
                        stackView.push(logoutScreenComponent, {"mode": mode})
                    }
                }
            }

            Component {
                id: logoutScreenComponent
                LogoutScreen {
                    onCancel: stackView.pop()

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
    
    function startLogin () {
        sddm.login(usersSelection.selectedUser, passwordInput.text, sessionCombo.currentIndex)
    }
}
