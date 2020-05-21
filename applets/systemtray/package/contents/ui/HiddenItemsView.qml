/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
 *   Copyright 2020 Konrad Materka <materka@gmail.com>
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
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import "items"

MouseArea {
    id: hiddenTasksView

    visible: !root.activeApplet || (root.activeApplet.parent && root.activeApplet.parent.inHiddenLayout)
    implicitWidth: root.activeApplet ? iconColumnWidth : parent.width
    property alias layout: hiddenTasksColumn
    //Useful to align stuff to the column of icons, both in expanded and shrink modes
    property int iconColumnWidth: root.hiddenItemSize + highlight.marginHints.left + highlight.marginHints.right

    hoverEnabled: true
    onExited: hiddenTasksColumn.currentIndex = -1

    PlasmaExtras.ScrollArea {
        width: parent.width
        height: parent.height
        frameVisible: false

        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
        verticalScrollBarPolicy: root.activeApplet ? Qt.ScrollBarAlwaysOff : Qt.ScrollBarAsNeeded

        ListView {
            id: hiddenTasksColumn

            spacing: units.smallSpacing

            currentIndex: -1
            highlight: PlasmaComponents.Highlight {}
            highlightMoveDuration: 0
            highlightResizeDuration: 0

            readonly property int iconItemHeight: root.hiddenItemSize + highlight.marginHints.top + highlight.marginHints.bottom
            property int itemCount: model.rowCount()

            model: PlasmaCore.SortFilterModel {
                sourceModel: plasmoid.nativeInterface.systemTrayModel
                filterRole: "effectiveStatus"
                filterCallback: function(source_row, value) {
                    return value === PlasmaCore.Types.PassiveStatus
                }
            }
            delegate: ItemLoader {}
        }
    }

    Connections {
        target: hiddenTasksColumn.model
        // hiddenTasksColumn.count is not updated when ListView is hidden and is not rendered
        // manually update itemCount so that expander arrow hides/shows itself correctly
        onModelReset: hiddenTasksColumn.itemCount = hiddenTasksColumn.model.rowCount()
        onRowsInserted: hiddenTasksColumn.itemCount = hiddenTasksColumn.model.rowCount()
        onRowsRemoved: hiddenTasksColumn.itemCount = hiddenTasksColumn.model.rowCount()
        onLayoutChanged: hiddenTasksColumn.itemCount = hiddenTasksColumn.model.rowCount()
    }

    PlasmaComponents.Highlight {
        id: highlight
        visible: false
    }

    CurrentItemHighLight {
        readonly property bool hiddenAppletActivated: root.activeApplet && root.activeApplet.parent && root.activeApplet.parent.inHiddenLayout
        parent: hiddenAppletActivated ? root.activeApplet.parent : hiddenTasksColumn.contentItem
        target: hiddenAppletActivated ? root.activeApplet.parent : null
        location: LayoutMirroring.enabled ? PlasmaCore.Types.RightEdge : PlasmaCore.Types.LeftEdge
    }
}
