/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras


PlasmaExtras.ScrollArea {
    id: hiddenTasksView

    visible: !activeApplet || activeApplet.parent.parent == hiddenTasksColumn
    width: activeApplet ? iconColumnWidth : parent.width
    property alias layout: hiddenTasksColumn
    //Useful to align stuff to the column of icons, both in expanded and shrink modes
    property int iconColumnWidth: root.hiddenItemSize + highlight.marginHints.left + highlight.marginHints.right
    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
    verticalScrollBarPolicy: activeApplet ? Qt.ScrollBarAlwaysOff : Qt.ScrollBarAsNeeded

    Flickable {
        contentWidth: width
        contentHeight: hiddenTasksColumn.height

        MouseArea {
            width: parent.width
            height: hiddenTasksColumn.height
            drag.filterChildren: true
            hoverEnabled: true
            onExited: hiddenTasksColumn.hoveredItem = null;

            CurrentItemHighLight {
                target: root.activeApplet && root.activeApplet.parent.parent == hiddenTasksColumn ? root.activeApplet.parent : null
                location: PlasmaCore.Types.LeftEdge
            }
            PlasmaComponents.Highlight {
                id: highlight
                visible: hiddenTasksColumn.hoveredItem != null && !root.activeApplet
                y: hiddenTasksColumn.hoveredItem ? hiddenTasksColumn.hoveredItem.y : 0
                width: hiddenTasksColumn.hoveredItem ? hiddenTasksColumn.hoveredItem.width : 0
                height: hiddenTasksColumn.hoveredItem ? hiddenTasksColumn.hoveredItem.height : 0
            }

            Column {
                id: hiddenTasksColumn

                spacing: units.smallSpacing
                width: parent.width
                property Item hoveredItem
                property alias marginHints: highlight.marginHints

                objectName: "hiddenTasksColumn"
            }
        }
    }
}
