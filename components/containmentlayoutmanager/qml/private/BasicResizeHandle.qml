/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick

import org.kde.plasma.private.containmentlayoutmanager as ContainmentLayoutManager
import org.kde.kirigami as Kirigami


// TODO: Add animations when showing/hiding the handles
// TODO: Add hover effect on the handles
// TODO: Extend clickable area of the handles for easier grabbing on touch
ContainmentLayoutManager.ResizeHandle {
    id: handle
    

    // FIXME: Remove before merge
    property bool debug: false
    
    readonly property bool isCorner: resizeCorner === ContainmentLayoutManager.ResizeHandle.TopLeft ||
                                     resizeCorner === ContainmentLayoutManager.ResizeHandle.TopRight ||
                                     resizeCorner === ContainmentLayoutManager.ResizeHandle.BottomLeft ||
                                     resizeCorner === ContainmentLayoutManager.ResizeHandle.BottomRight
        
    readonly property bool isHorizontalEdge: resizeCorner === ContainmentLayoutManager.ResizeHandle.Top ||
                                             resizeCorner === ContainmentLayoutManager.ResizeHandle.Bottom
    
    readonly property bool isVerticalEdge: resizeCorner === ContainmentLayoutManager.ResizeHandle.Left ||
                                           resizeCorner === ContainmentLayoutManager.ResizeHandle.Right

    
    width: isHorizontalEdge ? parent.width : Kirigami.Units.gridUnit * 2
    height: isVerticalEdge ? parent.height : Kirigami.Units.gridUnit * 2
    
    scale: overlay.open ? 1 : 0
    z: 999

    Item {
        anchors.fill: parent
        visible: handle.isCorner
        clip: true

        Rectangle {
            width: parent.width * 2
            height: parent.height * 2
            
            color: "transparent"
            
            border.width: Kirigami.Units.gridUnit / 4
            border.color: handle.resizeBlocked ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.highlightColor
            
            radius: Kirigami.Units.largeSpacing * 2
            
            x: {
                if (handle.resizeCorner === ContainmentLayoutManager.ResizeHandle.TopLeft ||
                    handle.resizeCorner === ContainmentLayoutManager.ResizeHandle.BottomLeft) {
                    return 0
                } else {
                    return -parent.width
                }
            }
            
            y: {
                if (handle.resizeCorner === ContainmentLayoutManager.ResizeHandle.TopLeft ||
                    handle.resizeCorner === ContainmentLayoutManager.ResizeHandle.TopRight) {
                    return 0
                } else {
                    return -parent.height
                }
            }
        }

    }

    Item {
        anchors.fill: parent
        visible: handle.isHorizontalEdge || handle.isVerticalEdge
        
        // FIXME: Remove before merge
        Rectangle {
            anchors.fill: parent
            color: handle.debug ? Qt.rgba(0, 1, 0, 0.3) : "transparent"
            border.width: handle.debug ? 1 : 0
            border.color: "green"
        }
    }
    
    // FIXME: Remove before merge
    Rectangle {
        anchors.fill: parent
        color: handle.debug && handle.isCorner ? Qt.rgba(1, 0, 0, 0.3) : "transparent"
        border.width: handle.debug && handle.isCorner ? 1 : 0
        border.color: "red"
        visible: handle.debug && handle.isCorner
        z: -1
    }

    Behavior on scale {
        NumberAnimation {
            duration: Kirigami.Units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
}

