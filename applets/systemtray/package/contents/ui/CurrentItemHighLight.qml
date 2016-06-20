/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
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
import org.kde.plasma.core 2.0 as PlasmaCore

PlasmaCore.FrameSvgItem {
    id: expandedItem

    property Item visualParent
    property Item target
    property int location

    x: Math.max(0, (target ? target.x : 0) + (visualParent ? visualParent.x : 0))
    y: Math.max(0, (target ? target.y : 0) + (visualParent ? visualParent.y : 0))
    width: Math.min(parent.width, target ? target.width : 0)
    height: Math.min(parent.height, target ? target.height : 0)

    imagePath: "widgets/tabbar"
    prefix: {
        var prefix = ""
        switch (location) {
            case PlasmaCore.Types.LeftEdge:
                prefix = "west-active-tab";
                break;
            case PlasmaCore.Types.TopEdge:
                prefix = "north-active-tab";
                break;
            case PlasmaCore.Types.RightEdge:
                prefix = "east-active-tab";
                break;
            default:
                prefix = "south-active-tab";
            }
            if (!hasElementPrefix(prefix)) {
                prefix = "active-tab";
            }
            return prefix;
        }
    opacity: target && dialog.visible ? 1 : 0
    Behavior on opacity {
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
    Behavior on x {
        enabled: target && expandedItem.opacity == 1
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
    Behavior on y {
        enabled: target && expandedItem.opacity == 1
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
    Behavior on width {
        enabled: target && expandedItem.opacity == 1
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
    Behavior on height {
        enabled: target && expandedItem.opacity == 1
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
}
