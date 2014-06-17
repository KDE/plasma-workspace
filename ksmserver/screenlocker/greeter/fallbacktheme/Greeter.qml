/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

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
import QtQuick.Layouts 1.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kscreenlocker 1.0

Item {
    id: root
    signal switchUserClicked()
    signal canceled()
    property alias notification: message.text
    property bool switchUserEnabled
    property bool capsLockOn
    implicitWidth: layoutItem.width + theme.mSize(theme.defaultFont).width * 4 + 12
    implicitHeight: layoutItem.height + 12

    anchors {
        fill: parent
        margins: 6
    }

    Column {
        id: layoutItem
        anchors.centerIn: parent
        spacing: theme.mSize(theme.defaultFont).height/2


        PlasmaComponents.Label {
            id: message
            text: ""
            anchors.horizontalCenter: parent.horizontalCenter
            font.bold: true
            Behavior on opacity {
                NumberAnimation {
                    duration: 250
                }
            }
            opacity: text == "" ? 0 : 1
        }

        PlasmaComponents.Label {
            id: capsLockMessage
            text: i18n("Warning: Caps Lock on")
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: capsLockOn ? 1 : 0
            height: capsLockOn ? paintedHeight : 0
            font.bold: true
            Behavior on opacity {
                NumberAnimation {
                    duration: 250
                }
            }
        }

        PlasmaComponents.Label {
            id: lockMessage
            text: kscreenlocker_userName.length == 0 ? i18n("The session is locked") : i18n("The session has been locked by %1", kscreenlocker_userName)
            anchors.horizontalCenter: parent.horizontalCenter
        }

        RowLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            PlasmaComponents.Label {
                text: i18n("Password:")
            }
            PlasmaComponents.TextField {
                id: password
                enabled: !authenticator.graceLocked
                echoMode: TextInput.Password
                focus: true
                Keys.onEnterPressed: authenticator.tryUnlock(password.text)
                Keys.onReturnPressed: authenticator.tryUnlock(password.text)
                Keys.onEscapePressed: password.text = ""
            }
        }

        PlasmaComponents.ButtonRow {
            id: buttonRow
            property bool showAccel: false
            exclusive: false
            spacing: theme.mSize(theme.defaultFont).width / 2
            anchors.horizontalCenter: parent.horizontalCenter

            AccelButton {
                id: switchUser
                label: i18nd("kscreenlocker_greet", "&Switch Users")
                iconSource: "fork"
                visible: switchUserEnabled
                onClicked: switchUserClicked()
            }

            AccelButton {
                id: unlock
                label: i18nd("kscreenlocker_greet", "Un&lock")
                iconSource: "object-unlocked"
                enabled: !authenticator.graceLocked
                onClicked: authenticator.tryUnlock(password.text)
            }
        }
    }

    Keys.onPressed: {
        var alt = (event.modifiers & Qt.AltModifier);
        buttonRow.showAccel = alt;

        if (alt) {
            // focus munging is needed otherwise the greet (QWidget)
            // eats all the key events, even if root is added to forwardTo
            // qml property of greeter
//             greeter.focus = false;
            root.forceActiveFocus();

            var buttons = [switchUser, unlock]
            for (var b = 0; b < buttons.length; ++b) {
                if (event.key == buttons[b].accelKey) {
                    buttonRow.showAccel = false;
                    buttons[b].clicked();
                    break;
                }
            }
        }
    }

    Keys.onReleased: {
        buttonRow.showAccel = (event.modifiers & Qt.AltModifier)
    }

    Connections {
        target: authenticator
        onFailed: {
            root.notification = i18nd("kscreenlocker_greet", "Unlocking failed");
        }
        onGraceLockedChanged: {
            if (!authenticator.graceLocked) {
                root.notification = "";
                password.selectAll();
                password.focus = true;
            }
        }
        onMessage: function(text) {
            root.notification = text;
        }
        onError: function(text) {
            root.notification = text;
        }
    }
}
