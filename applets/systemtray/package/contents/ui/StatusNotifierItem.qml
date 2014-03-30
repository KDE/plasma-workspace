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

    property int location: isHiddenItem ? PlasmaCore.Types.LeftEdge : plasmoid.location
    property int blink_interval: 1000 // interval of blinking (if status of task is NeedsAttention)
    property variant task: null // task that provides information for item

    //property bool     __has_task: task ? true : false
    property bool     __has_task: true
    property string   __icon_name:         __has_task ? iconName : ""
    property string   __att_icon_name:     __has_task ? attIconName : ""
    property variant  __icon:              __has_task ? icon : "default"
    property variant  __att_icon:          __has_task ? attIcon : __getDefaultIcon()
    property string   __overlay_icon_name: __has_task ? overlayIconName : ""
    property string   __movie_path:        __has_task ? moviePath : ""
    property int      __status:            __has_task ? status : SystemTray.Task.UnknownStatus


    property variant icon:    __has_task ? tooltipIcon : ""
    property string toolTipMainText: __has_task ? tooltipTitle : ""
    property string toolTipSubText:  __has_task ? tooltipText : ""

    // Public functions ================================================================================================
    function click(buttons) {
        __processClick(buttons, mouse_area)
    }

    function scrollHorz(delta) {
        activateHorzScroll(delta)
    }

    function scrollVert(delta) {
        activateVertScroll(delta)
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


    // Timer for blink effect ==========================================================================================
    Timer {
        id: timer_blink
        running: false
        repeat: true
        interval: blink_interval

        property bool is_att_icon: false

        onTriggered: {
            icon_widget.source = is_att_icon ? __getAttentionIcon() : __getDefaultIcon()
            is_att_icon = !is_att_icon
        }
    }
    
    PlasmaCore.IconItem {
        id: itemIcon
        width: isHiddenItem ? height * 1.5 : height;
        height: Math.min(parent.width, parent.height)
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
        source: iconName != "" ? iconName : (typeof(icon) != "undefined" ? icon : "")
    }

    // TODO: remove wheel area in QtQuick 2.0
    KQuickControlsAddonsComponents.MouseEventListener {
        id: wheel_area
        anchors.fill: parent
        enabled: __has_task
        visible: __has_task
        z: -2

        // Mouse events handlers ===========================================================================================
        MouseArea {
            id: mouse_area
            anchors.fill: parent
            hoverEnabled: true
            // if no task passed we don't accept any buttons, if icon_widget is visible we pass left button to it
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            enabled: __has_task
            visible: __has_task

            onClicked: __processClick(mouse.button, mouse_area)

            // Widget for icon if no animation is requested
            PlasmaCore.IconItem {
                id: icon_widget

                anchors.fill: parent

                property QtObject action: null; //FIXME: __has_task ? plasmoid.createShortcutAction(objectName + "-" + plasmoid.id) : null

                visible: false
                active: mouse_area.containsMouse
                source: iconName != "" ? iconName : (typeof(icon) != "undefined" ? icon : "")

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
        onWheelMoved: {
            //print("wheel moved by " + wheel.delta);
            if (wheel.orientation === Qt.Horizontal)
                activateHorzScroll(wheel.delta)
            else
                activateVertScroll(wheel.delta)
        }
    }

    // Functions ==================================================================================
    function __getDefaultIcon() {
        return task.customIcon(__icon_name != "" ? __icon_name : __icon)
    }

    function __getAttentionIcon() {
        return task.customIcon(__att_icon_name != "" ? __att_icon_name : __att_icon)
    }

    function __processClick(buttons, item) {
        print("__processClick");
        var pos = popupPosition(taskItemContainer, 0, 0);
        switch (buttons) {
        case Qt.LeftButton:
            root.expandedTask = null;
            activate1(pos.x, pos.y);
            break;
        case Qt.RightButton:
            activateContextMenu(pos.x, pos.y);
            break;
        case Qt.MiddleButton:
            activate2(pos.x, pos.y);
            break;
        }
        plasmoid.expanded = false;
    }

    // States =====================================================================================
    states: [
        // Static icon
        State {
            name: "__STATIC"
            when: __status !== NeedsAttention
            PropertyChanges {
                target: timer_blink
                running: false
            }
            PropertyChanges {
                target: icon_widget
                source: __getDefaultIcon()
                visible: true
            }
            PropertyChanges {
                target: animation
                visible: false
                playing: false
            }
            StateChangeScript {
                script: tooltip.target = icon_widget // binding to property doesn't work
            }
        },
        // Attention icon
        State {
            name: "__BLINK"
            when: __status === NeedsAttention && !__movie_path
            PropertyChanges {
                target: icon_widget
                source: __getAttentionIcon()
                visible: true
            }
            PropertyChanges {
                target: timer_blink
                running: true
                is_att_icon: false
            }
            PropertyChanges {
                target: animation
                visible: false
                playing: false
            }
            StateChangeScript {
                script: tooltip.target = icon_widget
            }
        },
        // Animation icon
        State {
            name: "__ANIM"
            when: __status === NeedsAttention && __movie_path
            PropertyChanges {
                target: timer_blink
                running: false
            }
            PropertyChanges {
                target: icon_widget
                source: __getDefaultIcon()
                visible: false
            }
            PropertyChanges {
                target: animation
                visible: true
                playing: true
            }
            StateChangeScript {
                script: tooltip.target = animation
            }
        }
    ]

}
