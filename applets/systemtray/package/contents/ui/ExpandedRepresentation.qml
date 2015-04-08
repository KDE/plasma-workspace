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
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.private.systemtray 2.0 as SystemTray

KQuickControlsAddons.MouseEventListener {

   acceptedButtons: Qt.RightButton

   Component.onCompleted: {
       if (root.expandedTask) {
           root.expandedTask.taskItemExpanded.parent = expandedItemContainer;
           root.expandedTask.taskItemExpanded.anchors.fill = expandedItemContainer;
           expandedItemContainer.replace(root.expandedTask.taskItemExpanded);
       } else {
           expandedItemContainer.clear();
       }
   }

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


    onPressed: {
        if (!root.expandedTask || expandedItemContainer.x > mouse.x || expandedItemContainer.x + expandedItemContainer.width < mouse.x) {
            return;
        }
        host.showMenu(mouse.screenX, mouse.screenY, root.expandedTask)
    }

    MouseArea {
        visible: hiddenView.visible
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
        clip: false
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
        visible: {
            // Normal system tray case; clicked on arrow
            if (root.expandedTask == null) {
                return true;
            // In case applet is hidden; we should show sidebar
            } else if (plasmoid.configuration.hiddenItems.indexOf(root.expandedTask.taskId) != -1) {
                return true;
            // In case applet is shown; we should not show sidebar
            } else if (plasmoid.configuration.shownItems.indexOf(root.expandedTask.taskId) != -1) {
                return false;
            }
            // At last verify the passive status of applet
            return root.expandedTask.status == SystemTray.Task.Passive;
        }

    }

    PlasmaCore.SvgItem {
        id: separator

        width: lineSvg.elementSize("vertical-line").width;
        visible: root.expandedTask != null && hiddenView.visible

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
        Behavior on opacity {
            NumberAnimation {}
            enabled: expandedItemContainer.animate
        }

        anchors {
            top: parent.top
            topMargin: units.gridUnit
            left: parent.left
            leftMargin: units.largeSpacing
            right: parent.right
        }
        text: i18n("Status & Notifications")
    }

    PlasmaExtras.Heading {
        id: snHeadingExpanded

        level: 1
        opacity: root.expandedTask != null ? 0.8 : 0
        Behavior on opacity {
            NumberAnimation {}
            enabled: expandedItemContainer.animate
        }

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
            left: hiddenView.visible ? separator.right : parent.left
            leftMargin: units.largeSpacing
            top: root.expandedTask != null ? snHeadingExpanded.bottom : snHeading.bottom
            topMargin: Math.round(units.largeSpacing / 2)
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

    PlasmaComponents.ToolButton {
        anchors.right: parent.right
        width: Math.round(units.gridUnit * 1.25)
        height: width
        checkable: true
        iconSource: "window-pin"
        onCheckedChanged: plasmoid.hideOnWindowDeactivate = !checked
    }

}
