/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.8
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components

PlasmaCore.Dialog {
    id: notificationPopup

    property int notificationId
    property string summary
    property string body
    property var icon
    property int timeout

    property bool hasDefaultAction

    signal closeClicked
    signal defaultActionInvoked
    signal actionInvoked(string actionName)
    signal expired

    location: PlasmaCore.Types.Floating
    type: PlasmaCore.Dialog.Notification
    flags: Qt.WindowDoesNotAcceptFocus

    visible: false

    mainItem: MouseArea {
        id: area
        width: units.gridUnit * 20//notificationItem.implicitWidth
        height: notificationItem.implicitHeight
        hoverEnabled: true

        cursorShape: hasDefaultAction ? Qt.PointingHandCursor : Qt.ArrowCursor
        acceptedButtons: hasDefaultAction ? Qt.LeftButton : Qt.NoButton

        onClicked: notificationPopup.defaultActionInvoked()

        Timer {
            id: timer
            interval: notificationPopup.timeout
            running: !area.containsMouse && notificationPopup.timeout > 0
            onTriggered: notificationPopup.expired()
        }

        NotificationItem {
            id: notificationItem
            width: parent.width
            summary: notificationPopup.summary
            body: notificationPopup.body
            icon: notificationPopup.icon

            closable: true // TODO with grouping and what not
            onCloseClicked: notificationPopup.closeClicked()
        }

        // FIXME so I can see whether hover handling actually works
        Rectangle {
            anchors {
                right: parent.right
            }
            color: "#f0f"
            width: 20
            height: 20
            visible: timer.running
        }

        Rectangle {
            anchors {
                right: parent.right
                rightMargin: 20
            }
            width: 20
            height: 20
            color: "#f00"
            visible: area.containsMouse
        }
    }
}
