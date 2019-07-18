/*
 *  Copyright 2019 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
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
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents

import org.kde.plasma.private.containmentlayoutmanager 1.0 as ContainmentLayoutManager 

import "private"

ContainmentLayoutManager.ConfigOverlay {
    id: overlay

    opacity: open
    Behavior on opacity {
        OpacityAnimator {
            duration: units.longDuration
            easing.type: Easing.InOutQuad
        }
    }

    MultiPointTouchArea {
        anchors.fill: parent
        property real previousMinX
        property real previousMinY
        property real previousMaxX
        property real previousMaxY
        property bool pinching: false
        mouseEnabled: false
        maximumTouchPoints: 2
        touchPoints: [
            TouchPoint { id: point1 },
            TouchPoint { id: point2 }
        ]

        onPressed: {
            overlay.itemContainer.layout.releaseSpace(overlay.itemContainer);
            previousMinX = point1.sceneX;
            previousMinY = point1.sceneY;
        }

        onUpdated: {
            var minX;
            var minY;
            var maxX;
            var maxY;

            if (point1.pressed && point2.pressed) {
                minX = Math.min(point1.sceneX, point2.sceneX);
                minY = Math.min(point1.sceneY, point2.sceneY);

                maxX = Math.max(point1.sceneX, point2.sceneX);
                maxY = Math.max(point1.sceneY, point2.sceneY);
            } else {
                minX = point1.pressed ? point1.sceneX : point2.sceneX;
                minY = point1.pressed ? point1.sceneY : point2.sceneY;
                maxX = -1;
                maxY = -1;
            }

            if (pinching == (point1.pressed && point2.pressed)) {
                overlay.itemContainer.x += minX - previousMinX;
                overlay.itemContainer.y += minY - previousMinY;

                if (pinching) {
                    overlay.itemContainer.width += maxX - previousMaxX + previousMinX - minX;
                    overlay.itemContainer.height += maxY - previousMaxY + previousMinY - minY;
                }
                overlay.itemContainer.layout.showPlaceHolderForItem(overlay.itemContainer);
            }

            pinching = point1.pressed && point2.pressed
            previousMinX = minX;
            previousMinY = minY;
            previousMaxX = maxX;
            previousMaxY = maxY;
        }
        onReleased: {
            if (point1.pressed || point2.pressed) {
                return;
            }
            overlay.itemContainer.layout.positionItem(overlay.itemContainer);
            overlay.itemContainer.layout.hidePlaceHolder();
            pinching = false
        }
        onCanceled: released()
    }

    BasicResizeHandle {
        resizeCorner: ContainmentLayoutManager.ResizeHandle.TopLeft
        anchors {
            horizontalCenter: parent.left
            verticalCenter: parent.top
        }
    }
    BasicResizeHandle {
        resizeCorner: ContainmentLayoutManager.ResizeHandle.Left
        anchors {
            horizontalCenter: parent.left
            verticalCenter: parent.verticalCenter
        }
    }
    BasicResizeHandle {
        resizeCorner: ContainmentLayoutManager.ResizeHandle.BottomLeft
        anchors {
            horizontalCenter: parent.left
            verticalCenter: parent.bottom
        }
    }
    BasicResizeHandle {
        resizeCorner: ContainmentLayoutManager.ResizeHandle.Bottom
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.bottom
        }
    }
    BasicResizeHandle {
        resizeCorner: ContainmentLayoutManager.ResizeHandle.BottomRight
        anchors {
            horizontalCenter: parent.right
            verticalCenter: parent.bottom
        }
    }
    BasicResizeHandle {
        resizeCorner: ContainmentLayoutManager.ResizeHandle.Right
        anchors {
            horizontalCenter: parent.right
            verticalCenter: parent.verticalCenter
        }
    }
    BasicResizeHandle {
        resizeCorner: ContainmentLayoutManager.ResizeHandle.TopRight
        anchors {
            horizontalCenter: parent.right
            verticalCenter: parent.top
        }
    }
    BasicResizeHandle {
        resizeCorner: ContainmentLayoutManager.ResizeHandle.Top
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.top
        }
    }
}

