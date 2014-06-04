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
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kscreenlocker 1.0

Item {
    property alias switchUserSupported: sessions.switchUserSupported
    implicitWidth: theme.mSize(theme.defaultFont).width * 55
    implicitHeight: theme.mSize(theme.defaultFont).height * 25
    signal activateSession()
    signal startNewSession()
    signal cancel()
    Sessions {
        id: sessions
    }
    anchors {
        fill: parent
        margins: 6
    }
    PlasmaExtras.ScrollArea {
        anchors {
            left: parent.left
            right: parent.right
            bottom: buttonRow.top
            bottomMargin: 5
        }
        height: parent.height - explainText.implicitHeight - buttonRow.height - 10

        ListView {
            model: sessions.model
            id: userSessionsView
            anchors.fill: parent

            delegate: PlasmaComponents.ListItem {
                content: PlasmaComponents.Label {
                    text: i18nc("thesession name and the location where the session is running (what vt)", "%1 (%2)", session, location)
                }
            }
            highlight: PlasmaComponents.Highlight {
                hover: true
                width: parent.width
            }
            focus: true
            MouseArea {
                anchors.fill: parent
                onClicked: userSessionsView.currentIndex = userSessionsView.indexAt(mouse.x, mouse.y)
                onDoubleClicked: {
                    sessions.activateSession(userSessionsView.indexAt(mouse.x, mouse.y));
                    activateSession();
                }
            }
        }
    }

    PlasmaComponents.Label {
        id: explainText
        text: i18n("The current session will be hidden " +
                    "and a new login screen or an existing session will be displayed.\n" +
                    "An F-key is assigned to each session; " +
                    "F%1 is usually assigned to the first session, " +
                    "F%2 to the second session and so on. " +
                    "You can switch between sessions by pressing " +
                    "Ctrl, Alt and the appropriate F-key at the same time. " +
                    "Additionally, the KDE Panel and Desktop menus have " +
                    "actions for switching between sessions.",
                    7, 8)
        wrapMode: Text.Wrap
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
    }
    PlasmaComponents.ButtonRow {
        id: buttonRow
        exclusive: false
        spacing: theme.mSize(theme.defaultFont).width / 2
        property bool showAccel: false

        AccelButton {
            id: activateSession
            label: i18n("Activate")
            iconSource: "fork"
            onClicked: {
                sessions.activateSession(userSessionsView.currentIndex);
                activateSession();
            }
        }
        AccelButton {
            id: newSession
            label: i18n("Start New Session")
            iconSource: "fork"
            visible: sessions.startNewSessionSupported
            onClicked: {
                sessions.startNewSession();
                startNewSession();
            }
        }
        AccelButton {
            id: cancelSession
            label: i18n("Cancel")
            iconSource: "dialog-cancel"
            onClicked: cancel()
        }
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: userSessionsUI.horizontalCenter
    }

    Keys.onPressed: {
        var alt = (event.modifiers & Qt.AltModifier);
        buttonRow.showAccel = alt;

        if (alt) {
            var buttons = [activateSession, newSession, cancelSession];
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
}
