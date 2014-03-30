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
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddonsComponents

import org.kde.private.systemtray 2.0 as SystemTray
//import "plasmapackage:/code/Layout.js" as LayoutManager


KQuickControlsAddonsComponents.MouseEventListener {
    id: taskItemContainer
    objectName: "taskItemContainer"

    height: root.itemSize + (units.smallSpacing * 2)
    width: snExpanded ? parent.width : height

    hoverEnabled: true

    property variant task: null
    property bool isCurrentTask: (root.expandedTask == modelData)

    property bool isHiddenItem: false
    property int location: plasmoid.location



    onClicked: {
        if (taskType == SystemTray.Task.TypePlasmoid) {
            togglePopup();
        }
    }

    Timer {
        id: hidePopupTimer
        interval: 10
        running: false
        repeat: false
        onTriggered: {
            print("hidetimer triggered, collapsing " + (root.expandedTask == null) )
            if (root.expandedTask == null) {
                plasmoid.expanded = false
            }
        }
    }

    // opacity is raised when: plasmoid is collapsed, we are the current task, or it's hovered
    opacity: (containsMouse || !plasmoid.expanded || isCurrentTask) || (plasmoid.expanded && root.expandedTask == null) ? 1.0 : 0.6
    Behavior on opacity { NumberAnimation { duration: units.shortDuration * 3 } }


    property int taskStatus: status
    property int taskType: type
    property Item expandedItem: taskItemExpanded
    property Item expandedStatusItem: null
    property bool snExpanded: false

    Rectangle {
        anchors.fill: parent;
        border.width: 1;
        border.color: "black";
        color: "yellow";
        visible: root.debug;
        opacity: 0.8;
    }

    property bool isExpanded: expanded

    onIsExpandedChanged: {
        if (expanded) {
            var task;
            if (root.expandedTask) {
                task = root.expandedTask;
            }
            root.expandedTask = modelData;
            if (task) {
                task.expanded = false;
            }
        } else if (root.expandedTask == modelData) {
            root.expandedTask = null;
        }
    }


    PulseAnimation {
        targetItem: taskItemContainer
        running: status == SystemTray.Task.NeedsAttention
    }

    onWidthChanged: updatePlasmoidGeometry()
    onHeightChanged: updatePlasmoidGeometry()

    function updatePlasmoidGeometry() {
        if (taskItem != undefined) {
            var _size = Math.min(taskItemContainer.width, taskItemContainer.height);
            var _m = (taskItemContainer.height - _size) / 2
            taskItem.anchors.verticalCenter = taskItemContainer.verticalCenter;
            taskItem.x = 0;
            taskItem.height = _size;
            taskItem.width = isHiddenItem ? _size * 1.5 : _size;
        }
    }

    PlasmaCore.ToolTipArea {
        anchors.fill: parent
        icon: taskItem ? taskItem.icon : sniLoader.item.icon
        mainText: taskItem ? taskItem.toolTipMainText : sniLoader.item.toolTipMainText
        subText: taskItem ? taskItem.toolTipSubText : sniLoader.item.toolTipSubText
        location: taskItem ? taskItemContainer.location : sniLoader.item.location
        Loader {
            id: sniLoader
            anchors.fill: parent
        }
    }

    Component.onCompleted: {
        if (taskType == SystemTray.Task.TypeStatusItem) {
            sniLoader.source = "StatusNotifierItem.qml";

        } else if (taskItem != undefined) {
            sniLoader.source = "PlasmoidItem.qml";
            taskItem.parent = taskItemContainer;
            taskItem.z = 999;
            updatePlasmoidGeometry();
        }
    }
}
