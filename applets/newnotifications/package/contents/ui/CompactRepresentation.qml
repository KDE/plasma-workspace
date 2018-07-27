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

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

MouseArea {
    id: compactRoot

    property bool wasExpanded: false
    onPressed: wasExpanded = plasmoid.expanded
    onClicked: plasmoid.expanded = !wasExpanded

    PlasmaCore.Svg {
        id: notificationSvg
        imagePath: "icons/notification"
        colorGroup: PlasmaCore.ColorScope.colorGroup
    }

    PlasmaCore.SvgItem {
        anchors.centerIn: parent
        width: units.roundToIconSize(Math.min(parent.width, parent.height))
        height: width
        svg: notificationSvg

        // TODO icon depending on unread notifications, active jobs, etc
        // TODO use States for the icon handling including animation and what not?
        elementId: {
            return "notification-disabled"
        }

        // FIXME just so I can tell the two apart in system tray
        Text {
            anchors.fill: parent
            text: "N" // "New"
        }
    }

    // TODO progress
    // would be lovely if we could get back the circular pie thing back we had in Plasma < 4.10

}
