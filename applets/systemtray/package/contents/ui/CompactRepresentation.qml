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
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddonsComponents

import org.kde.private.systemtray 2.0 as SystemTray


Item {
    id: compactRepresentation

    Layout.minimumWidth: !root.vertical ? computeDimension() : computeDimensionHeight()
    Layout.minimumHeight: root.vertical ? computeDimension() : computeDimensionHeight()
    Layout.maximumWidth: Layout.minimumWidth
    Layout.maximumHeight: Layout.minimumHeight
    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight

    Layout.fillWidth: false
    Layout.fillHeight: false

    property QtObject systrayhost: undefined

    Connections {
        target: root
        onExpandedTaskChanged: {
            parent.expandedFeedback.visible = (root.expandedTask == null)
        }
    }

    Timer {
        id: hidePopupTimer
        interval: 10
        running: false
        repeat: false
        onTriggered: {
            //print("hidetimer triggered, collapsing " + (root.expandedTask == null) )
            if (root.expandedTask == null) {
                plasmoid.expanded = false
            }
        }
    }

    function computeDimension() {
        var dim = root.vertical ? compactRepresentation.width : compactRepresentation.height
        var rows = Math.floor(dim / root.itemSize);
        var cols = Math.ceil(systrayhost.shownTasks.length / rows);
        var res = cols * (root.itemSize + units.smallSpacing*4) + units.smallSpacing + tooltip.width;
        return res;
    }

    function computeDimensionHeight() {
        var dim = root.vertical ? compactRepresentation.width : compactRepresentation.height
        var rows = Math.floor(dim / root.itemSize);
        var rr = rows * (root.itemSize + units.smallSpacing);
        return rr;
    }

    function togglePopup() {
        //print("toggle popup => " + !plasmoid.expanded);
        if (!plasmoid.expanded) {
            plasmoid.expanded = true
        } else {
            hidePopupTimer.start();
        }
    }

    Rectangle {
        anchors.fill: parent;
        border.width: 1;
        border.color: "black";
        color: "green";
        visible: root.debug;
        opacity: 0.2;
    }

    Component {
        id: taskDelegateComponent
        TaskDelegate {
            id: taskDelegate
            //task: ListView.view.model
        }
    }

    GridView {
        id: gridView
        objectName: "gridView"
        flow: !root.vertical ? GridView.LeftToRight : GridView.TopToBottom

        anchors {
            top: parent.top
            bottom: parent.bottom
           // topMargin: !root.vertical ? ((parent.height - root.itemSize) / 2) - units.smallSpacing : units.smallSpacing
            left: parent.left
            //leftMargin: root.vertical ? 0 : units.smallSpacing
            right: tooltip.left
        }
        cellWidth: !root.vertical ? root.itemSize + units.smallSpacing * 4 : root.itemSize
        cellHeight: root.vertical ? root.itemSize + units.smallSpacing * 4 : root.itemSize
        interactive: false

        model: systrayhost.shownTasks

        delegate: taskDelegateComponent
    }

    // Tooltip for arrow --------------------------------
    PlasmaCore.ToolTipArea {
        id: tooltip

        width: root.vertical ? compactRepresentation.width : units.iconSizes.smallMedium
        height: !root.vertical ? compactRepresentation.height : units.iconSizes.smallMedium
        anchors {
            right: parent.right
            bottom: parent.bottom
        }

        subText: plasmoid.expanded ? i18n("Hide icons") : i18n("Show hidden icons")

        MouseArea {
            anchors.fill: parent
            onClicked: plasmoid.expanded = !plasmoid.expanded

            PlasmaCore.SvgItem {
                id: arrow

                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: width

                svg: PlasmaCore.Svg { imagePath: "widgets/arrows" }
                elementId: {

                    var exp = plasmoid.expanded; // flip for bottom edge and right edge

                    if (plasmoid.location == PlasmaCore.Types.BottomEdge) {
                        return (exp) ? "down-arrow" : "up-arrow"
                    } else if (plasmoid.location == PlasmaCore.Types.TopEdge) {
                        return (exp) ? "up-arrow" : "down-arrow"
                    } else if (plasmoid.location == PlasmaCore.Types.LeftEdge) {
                        return (exp) ? "left-arrow" : "right-arrow"
                    } else {
                        return (exp) ? "right-arrow" : "left-arrow"
                    }

                }
            }
        }
    }

    onHeightChanged: ttt.running = true
    onWidthChanged: ttt.running = true

    Timer {
        id: ttt
        interval: 50
        running: false
        repeat: false
        onTriggered: {
            var dim = root.vertical ? compactRepresentation.width : compactRepresentation.height;
            root.preferredItemSize = dim - units.smallSpacing;
        }
    }
}
