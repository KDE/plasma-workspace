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

import QtQuick 2.12
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

PlasmaCore.FrameSvgItem {
    id: expandedItem

    property int location
    property bool animationEnabled: true

    z: -1 // always draw behind icons
    width: parent.width
    height: parent.height
    opacity: parent && systemTrayState.expanded ? 1 : 0

    function changeHighlightedItem(nextItem) {
        parent = nextItem;
    }

    function changeHighlightedItemNoAnimation(nextItem) {
        animationEnabled = false;
        parent = nextItem;
        animationEnabled = true;
    }

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

    Connections {
        target: systemTrayState

        function onActiveAppletChanged() {
            if (systemTrayState.activeApplet && systemTrayState.activeApplet.parent.inVisibleLayout) {
                changeHighlightedItem(systemTrayState.activeApplet.parent.container)
            } else if (systemTrayState.expanded) {
                changeHighlightedItem(root)
            }
        }

        function onExpandedChanged() {
            if (systemTrayState.expanded && !systemTrayState.activeApplet) {
                changeHighlightedItemNoAnimation(root)
            }
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: units.longDuration
            easing.type: parent && systemTrayState.expanded ? Easing.OutCubic : Easing.InCubic
        }
    }
    Behavior on x {
        id: xAnim
        enabled: parent && animationEnabled
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutCubic
        }
    }
    Behavior on y {
        id: yAnim
        enabled: parent && animationEnabled
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutCubic
        }
    }
    Behavior on width {
        id: widthAnim
        enabled: parent && animationEnabled
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutCubic
        }
    }
    Behavior on height {
        id: heightAnim
        enabled: parent && animationEnabled
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutCubic
        }
    }
}
