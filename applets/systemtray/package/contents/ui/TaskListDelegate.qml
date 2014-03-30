/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

import org.kde.private.systemtray 2.0 as SystemTray


TaskDelegate {
    id: taskListDelegate
    objectName: "taskListDelegate"

    snExpanded: (root.expandedTask == null)

    width: snExpanded ? parent.width : height * 1.5 // be a bit more lenient to input
    height: (root.baseSize * 2)

    isHiddenItem: true
    location: PlasmaCore.Types.LeftEdge

    PlasmaComponents.Highlight {
        anchors.fill: parent
        //anchors.rightMargin: height
        anchors.margins: -units.smallSpacing
        opacity: containsMouse && snExpanded ? 1 : 0
        Behavior on opacity { NumberAnimation {} }
    }

    PlasmaComponents.Label {
        id: mainLabel

        anchors {
            left: parent.left
            leftMargin: parent.height + units.largeSpacing
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        opacity: taskListDelegate.snExpanded ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: units.longDuration } }

        text: name
        elide: Text.ElideRight
    }
}
