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
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.private.systemtray 2.0 as SystemTray

Item {

    Connections {
        target: root
        onExpandedTaskChanged: {
            if (root.expandedTask) {
                root.expandedTask.taskItemExpanded.parent = expandedItemContainer;
                root.expandedTask.taskItemExpanded.anchors.fill = expandedItemContainer;
                expandedItemContainer.replace(root.expandedTask.taskItemExpanded);
            } else {
                expandedItemContainer.clear();
            }
        }
    }

    MouseArea {
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            right: expandedItemContainer.left
        }
        onClicked: {
            expandedItemContainer.clear();
            if (root.expandedTask) {
                root.expandedTask.expanded = false;
                root.expandedTask = null;
            }
        }
    }

    ListView {
        id: hiddenView
        objectName: "hiddenView"
        clip: true
        width: parent.width

        interactive: (contentHeight > height)

        anchors {
            top: snHeading.bottom
            topMargin: units.largeSpacing / 2
            bottom: parent.bottom
            left: parent.left
        }
        spacing: units.smallSpacing

        model: host.hiddenTasks

        delegate: TaskListDelegate {}
    }

    PlasmaCore.SvgItem {
        id: separator

        width: lineSvg.elementSize("vertical-line").width;
        visible: root.expandedTask != null

        anchors {
            left: parent.left
            leftMargin: root.baseSize * 3 - 1
            bottom: parent.bottom;
            top: parent.top;
            //TODO: if this line will make it to the final design, measures have to come from FrameSvg
            topMargin: -4
            bottomMargin: -4
        }
        elementId: "vertical-line";

        svg: PlasmaCore.Svg {
            id: lineSvg;
            imagePath: "widgets/line";
        }
    }

    PlasmaExtras.Heading {
        id: snHeading

        level: 1
        opacity: root.expandedTask != null ? 0 : 0.8
        Behavior on opacity { NumberAnimation {} }

        anchors {
            top: parent.top
            topMargin: units.gridUnit
            leftMargin: -units.gridUnit
            left: expandedItemContainer.left
            right: parent.right
        }
        text: i18n("Status & Notifications")
    }

    PlasmaExtras.Heading {
        id: snHeadingExpanded

        level: 1
        opacity: root.expandedTask != null ? 0.8 : 0
        Behavior on opacity { NumberAnimation {} }

        anchors {
            top: parent.top
            topMargin: units.gridUnit
            left: expandedItemContainer.left
            right: parent.right
        }
        text: root.expandedTask ? root.expandedTask.name : ""
    }

    PlasmaComponents.PageStack {
        id: expandedItemContainer
        animate: false
        anchors {
            left: parent.left
            leftMargin: root.baseSize * 3 + units.largeSpacing
            top: snHeading.bottom
            topMargin: units.largeSpacing / 2
            bottom: parent.bottom
            right: parent.right
        }

        Timer {
            // this mechanism avoids animating the page switch
            // when the popup is still closed.
            interval: 500
            running: plasmoid.expanded
            onTriggered: expandedItemContainer.animate = true
        }
    }

    Connections {
        target: plasmoid
        onExpandedChanged: {
            if (!plasmoid.expanded) {
                expandedItemContainer.animate = false;
            }
        }

    }

    MouseArea {
        id: pin

        /* Allows the user to keep the calendar open for reference */

        width: units.largeSpacing
        height: width
        hoverEnabled: true
        anchors {
            top: parent.top
            right: parent.right
        }

        property bool checked: false

        onClicked: {
            pin.checked = !pin.checked;
            plasmoid.hideOnWindowDeactivate = !pin.checked;
        }

        PlasmaCore.IconItem {
            anchors.centerIn: parent
            source: pin.checked ? "window-unpin" : "window-pin"
            width: units.iconSizes.small / 2
            height: width
            active: pin.containsMouse
        }
    }
}