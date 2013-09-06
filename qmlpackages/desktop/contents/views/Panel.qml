/*
 *  Copyright 2012 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
//import org.kde.plasma 2.0

import org.kde.plasma.core 2.0 as PlasmaCore


PlasmaCore.FrameSvgItem {
    id: root
    width: 640
    height: 32
    imagePath: "widgets/panel-background"

    property Item containment

    function adjustBorders() {
        if (!containment) {
            return;
        }
        var borders = PlasmaCore.FrameSvg.AllBorders;

        switch (containment.location) {
        case PlasmaCore.Types.TopEdge:
            borders = borders & ~PlasmaCore.FrameSvg.TopBorder;
            break;
        case PlasmaCore.Types.LeftEdge:
            borders = borders & ~PlasmaCore.FrameSvg.LeftBorder;
            break;
        case PlasmaCore.Types.RightEdge:
            borders = borders & ~PlasmaCore.FrameSvg.RightBorder;
            break;
        case PlasmaCore.Types.BottomEdge:
        default:
            borders = borders & ~PlasmaCore.FrameSvg.BottomBorder;
            break;
        }

        if (panel.x <= panel.screen.geometry.x) {
            borders = borders & ~PlasmaCore.FrameSvg.LeftBorder;
        }
        if (panel.x + panel.width >= panel.screen.geometry.x + panel.screen.geometry.width) {
            borders = borders & ~PlasmaCore.FrameSvg.RightBorder;
        }
        if (panel.y <= panel.screen.geometry.y) {
            borders = borders & ~PlasmaCore.FrameSvg.TopBorder;
        }
        if (panel.y + panel.height >= panel.screen.geometry.y + panel.screen.geometry.height) {
            borders = borders & ~PlasmaCore.FrameSvg.BottomBorder;
        }

        root.enabledBorders = borders;
    }

    onContainmentChanged: {
        print("New panel Containment: " + containment)
        //containment.parent = root
        containment.visible = true
        containment.anchors.fill = root
    }

    Connections {
        target: containment
        onLocationChanged: {
            adjustBorders()
        }
        onMinimumWidthChanged: {
            if (containment.formFactor === PlasmaCore.Types.Horizontal) {
                panel.width = Math.max(panel.width, panel.minimumLength);
            }
        }
        onMaximumWidthChanged: {
            if (containment.formFactor === PlasmaCore.Types.Horizontal) {
                panel.width = Math.min(panel.width, panel.maximumLength);
            }
        }
        onImplicitWidthChanged: {
            if (containment.formFactor === PlasmaCore.Types.Horizontal) {
                panel.width = Math.min(panel.maximumLength, Math.max(containment.implicitWidth, panel.minimumLength));
            }
        }

        onMinimumHeightChanged: {
            if (containment.formFactor === PlasmaCore.Types.Vertical) {
                panel.width = Math.max(panel.width, panel.minimumLength);
            }
        }
        onMaximumHeightChanged: {
            if (containment.formFactor === PlasmaCore.Types.Vertical) {
                panel.width = Math.min(panel.width, panel.maximumLength);
            }
        }
        onImplicitHeightChanged: {
            if (containment.formFactor === PlasmaCore.Types.Vertical) {
                panel.width = Math.min(panel.maximumLength, Math.max(containment.implicitHeight, panel.minimumLength));
            }
        }
    }

    Connections {
        target: panel
        onXChanged: {
            adjustBorders();
        }
        onYChanged: {
            adjustBorders();
        }
        onWidthChanged: {
            adjustBorders();
        }
        onHeightChanged: {
            adjustBorders();
        }
    }

    Connections {
        target: panel.screen
        onGeometryChanged: {
            adjustBorders();
        }
    }

    Component.onCompleted: {
        print("PanelView QML loaded")
    }
}
