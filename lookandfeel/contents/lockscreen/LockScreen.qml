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
import org.kde.kscreenlocker 1.0
import org.kde.plasma.workspace.keyboardlayout 1.0
import "../components"
import "../osd"

Image {
    id: root
    property bool debug: false
    property string notification
    property UserSelect userSelect: null
    signal clearPassword()

    source: backgroundPath || "../components/artwork/background.png"
    fillMode: Image.PreserveAspectCrop

    onStatusChanged: {
        if (status == Image.Error) {
            source = "../components/artwork/background.png";
        }
    }

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
        onMessage: {
            root.notification = msg;
        }
        onError: {
            root.notification = err;
        }
    }
    Sessions {
        id: sessions
    }

    PlasmaCore.DataSource {
        id: keystateSource
        engine: "keystate"
        connectedSources: "Caps Lock"
    }

    StackView {
        id: stackView
        height: units.largeSpacing * 14
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
                        if(sessions.startNewSessionSupported) {
                            users.append({realName: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "New Session"),
                                          icon: "system-log-out", //TODO Need an icon for new session
                                          ButtonLabel: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Create Session"),
                                          ButtonAction: "newSession"
                            })
                        }
                        if(sessions.switchUserSupported) {
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

                        KeyboardLayoutButton {}

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
                            visible: block.mainItem.model.count > 0 ? !!block.mainItem.model.get(block.mainItem.selectedIndex).showPassword : false
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
                        }

                        PlasmaComponents.Button {
                            id: actionButton
                            Layout.minimumWidth: passwordInput.width
                            text: block.mainItem.model.count > 0 ? block.mainItem.model.get(block.mainItem.selectedIndex).ButtonLabel : ""
                            enabled: !authenticator.graceLocked
                            onClicked: switch(block.mainItem.model.get(block.mainItem.selectedIndex)["ButtonAction"]) {
                                case "unlock":
                                    unlockFunction();
                                    break;
                                case "newSession":
                                    sessions.startNewSession();
                                    break;
                                case "changeSession":
                                    stackView.push(changeSessionComponent)
                                    break;
                            }
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
    PlasmaCore.FrameSvgItem {
        id: osd

        // OSD Timeout in msecs - how long it will stay on the screen
        property int timeout: 1800
        // This is either a text or a number, if showingProgress is set to true,
        // the number will be used as a value for the progress bar
        property var osdValue
        // Icon name to display
        property string icon
        // Set to true if the value is meant for progress bar,
        // false for displaying the value as normal text
        property bool showingProgress: false

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }

        objectName: "onScreenDisplay"
        visible: false
        width: osdItem.width + margins.left + margins.right
        height: osdItem.height + margins.top + margins.bottom
        imagePath: "widgets/background"

        function show() {
            osd.visible = true;
            hideTimer.restart();
        }

        OsdItem {
            id: osdItem
            rootItem: osd

            anchors.centerIn: parent
        }

        Timer {
            id: hideTimer
            interval: osd.timeout
            onTriggered: {
                osd.visible = false;
                osd.icon = "";
                osd.osdValue = 0;
            }
        }
    }
}
