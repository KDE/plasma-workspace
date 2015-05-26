/***********************************************************************************************************************
 * KDE System Tray (Plasmoid)
 *
 * Copyright (C) 2012 ROSA  <support@rosalab.ru>
 * License: LGPLv2+
 * Authors: Dmitry Ashkadov <dmitry.ashkadov@rosalab.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **********************************************************************************************************************/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddonsComponents
import org.kde.private.systemtray 2.0 as SystemTray


Item {
    id: root_item

    property int location: isHiddenItem ? (plasmoid.location == PlasmaCore.Types.LeftEdge ? PlasmaCore.Types.LeftEdge : PlasmaCore.Types.RightEdge) : plasmoid.location
    property int blink_interval: 1000 // interval of blinking (if status of task is NeedsAttention)
    property variant task: null // task that provides information for item
    property bool useAttentionIcon: false

    property string   __icon_name:         modelData.iconName
    property string   __att_icon_name:     modelData.attIconName
    property variant  __icon:              modelData.icon
    property variant  __att_icon:          modelData.attIcon
    property string   __overlay_icon_name: modelData.overlayIconName
    property string   __movie_path:        modelData.moviePath
    property int      __status:            modelData.status


    property variant icon: modelData ? modelData.tooltipIcon : null
    property string toolTipMainText: modelData.tooltipTitle
    property string toolTipSubText: modelData.tooltipText

    // Public functions ================================================================================================
    function click(buttons) {
        __processClick(buttons, mouse_area)
    }

    function scrollHorz(delta) {
        modelData.activateHorzScroll(delta)
    }

    function scrollVert(delta) {
        modelData.activateVertScroll(delta)
    }

    function getMouseArea() {
        return mouse_area
    }
//     Connections {
//         target: task
//
//         onChangedShortcut: {
//             // update shortcut for icon widget
//             if (!icon_widget.action)
//                 return
//             plasmoid.updateShortcutAction(icon_widget.action, task.shortcut)
//             icon_widget.action.triggered.disconnect(__onActivatedShortcut) // disconnect old signals
//             icon_widget.action.triggered.connect(__onActivatedShortcut)
//         }
//
//         onShowContextMenu: plasmoid.showMenu(menu, x, y, root_item)
//     }
    function __onActivatedShortcut() {
        __processClick(Qt.LeftButton, icon_widget)
    }

    PlasmaCore.IconItem {
        id: itemIcon
        width: isHiddenItem ? height * 1.5 : height;
        height: Math.min(parent.width, parent.height)
        anchors {
            centerIn: isHiddenItem ? undefined : parent
            left: isHiddenItem ? parent.left : undefined
            verticalCenter: parent.verticalCenter               
        }
        source: useAttentionIcon ? __getAttentionIcon() : __getDefaultIcon()
    }

    // Mouse events handlers ===========================================================================================
    MouseArea {
        id: mouse_area
        anchors.fill: parent
        hoverEnabled: true
        // if no task passed we don't accept any buttons, if icon_widget is visible we pass left button to it
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        enabled: true
        visible: true

        onClicked: __processClick(mouse.button, mouse_area)
        onWheel: {
            //don't send activateVertScroll with a delta of 0, some clients seem to break (kmix)
            if (wheel.angleDelta.y !== 0) {
                modelData.activateVertScroll(wheel.angleDelta.y)
            }
            if (wheel.angleDelta.x !== 0) {
                modelData.activateHorzScroll(wheel.angleDelta.x)
            }
        }

        // Widget for icon if no animation is requested
        PlasmaCore.IconItem {
            id: icon_widget

            anchors.fill: parent

            property QtObject action: null; //FIXME: __has_task ? plasmoid.createShortcutAction(objectName + "-" + plasmoid.id) : null

            visible: false
            active: mouse_area.containsMouse
            source: useAttentionIcon ? __getAttentionIcon() : __getDefaultIcon()

            // Overlay icon
            Image {
                width: 10  // we fix size of an overlay icon
                height: width
                anchors { right: parent.right; bottom: parent.bottom }

                sourceSize.width: width
                sourceSize.height: width
                fillMode: Image.PreserveAspectFit
                smooth: true
                source: "image://icon/" + __overlay_icon_name
                visible: __overlay_icon_name
            }

            Component.onDestruction: {
                var act = icon_widget.action
                if (act != null) {
                    icon_widget.action = null
                    plasmoid.destroyShortcutAction(act)
                }
            }
        }

        // Animation (Movie icon)
        AnimatedImage {
            id: animation

            anchors.fill: parent

            playing: false
            visible: false
            smooth: true
            source: __movie_path
        }
    }

    // Functions ==================================================================================
    function __getDefaultIcon() {
        return __icon_name != "" ? __icon_name : (typeof(__icon) != "undefined" ? __icon : "")
    }

    function __getAttentionIcon() {
        return __att_icon_name != "" ? __att_icon_name : (typeof(__att_icon) != "undefined" ? __att_icon : "")
    }

    function __processClick(buttons, item) {
        taskItemContainer.hideToolTip();
        var pos = modelData.popupPosition(taskItemContainer, 0, 0);
        switch (buttons) {
        case Qt.LeftButton:
            root.expandedTask = null;
            modelData.activate1(pos.x, pos.y);
            plasmoid.expanded = false;
            break;
        case Qt.RightButton:
            modelData.activateContextMenu(pos.x, pos.y);
            break;
        case Qt.MiddleButton:
            modelData.activate2(pos.x, pos.y);
            break;
        }
    }
}
