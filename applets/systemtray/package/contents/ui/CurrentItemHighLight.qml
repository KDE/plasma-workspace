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
    id: currentItemHighLight

    property int location

    property bool animationEnabled: true
    property var highlightedItem: null

    z: -1 // always draw behind icons
    opacity: systemTrayState.expanded ? 1 : 0

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
            updateHighlightedItem();
        }

        function onExpandedChanged() {
            updateHighlightedItem();
        }
    }

    function updateHighlightedItem() {
        if (systemTrayState.expanded) {
            if (systemTrayState.activeApplet && systemTrayState.activeApplet.parent.inVisibleLayout) {
                changeHighlightedItem(systemTrayState.activeApplet.parent.container);
            } else { // 'Show hiden items' popup
                changeHighlightedItem(parent);
            }
        } else {
            highlightedItem = null;
        }
    }

    function changeHighlightedItem(nextItem) {
        if (!highlightedItem) {
            // do not animate the first appearance
            animationEnabled = false;
        }

        highlightedItem = nextItem;

        const p = parent.mapFromItem(highlightedItem, 0, 0)
        x = p.x;
        y = p.y;
        width = highlightedItem.width
        height = highlightedItem.height

        animationEnabled = true;
    }

    Behavior on opacity {
        NumberAnimation {
            duration: PlasmaCore.Units.longDuration
            easing.type: systemTrayState.expanded ? Easing.OutCubic : Easing.InCubic
        }
    }
    Behavior on x {
        id: xAnim
        enabled: animationEnabled
        NumberAnimation {
            duration: PlasmaCore.Units.longDuration
            easing.type: Easing.InOutCubic
        }
    }
    Behavior on y {
        id: yAnim
        enabled: animationEnabled
        NumberAnimation {
            duration: PlasmaCore.Units.longDuration
            easing.type: Easing.InOutCubic
        }
    }
    Behavior on width {
        id: widthAnim
        enabled: animationEnabled
        NumberAnimation {
            duration: PlasmaCore.Units.longDuration
            easing.type: Easing.InOutCubic
        }
    }
    Behavior on height {
        id: heightAnim
        enabled: animationEnabled
        NumberAnimation {
            duration: PlasmaCore.Units.longDuration
            easing.type: Easing.InOutCubic
        }
    }
}
