/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick

import org.kde.kirigami as Kirigami

import org.kde.plasma.private.containmentlayoutmanager as ContainmentLayoutManager

ContainmentLayoutManager.ConfigOverlay {
    id: overlay

    LayoutMirroring.enabled: false
    LayoutMirroring.childrenInherit: true

    opacity: open
    Behavior on opacity {
        OpacityAnimator {
            duration: Kirigami.Units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
    
    Rectangle {
        anchors {
            fill: parent
            margins: -(Kirigami.Units.gridUnit * 1.5)
        }
        color: !overlay.placeholderActive ? Qt.rgba(Kirigami.Theme.disabledTextColor.r, 
                              Kirigami.Theme.disabledTextColor.g, 
                              Kirigami.Theme.disabledTextColor.b, 
                              0.1) : "transparent"
        radius: Kirigami.Units.largeSpacing * 2

        Behavior on color {
            enabled: true
            ColorAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutCubic
            }
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
            let minX;
            let minY;
            let maxX;
            let maxY;

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

            if (pinching === (point1.pressed && point2.pressed)) {
                overlay.placeholderActive = true;
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
            pinching = false;
            overlay.placeholderActive = false;
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
        resizeCorner: ContainmentLayoutManager.ResizeHandle.Top
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.top
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
        resizeCorner: ContainmentLayoutManager.ResizeHandle.Left
        anchors {
            top: parent.top
            bottom: parent.bottom
            horizontalCenter: parent.left
        }
    }
    BasicResizeHandle {
        resizeCorner: ContainmentLayoutManager.ResizeHandle.Right
        anchors {
            top: parent.top
            bottom: parent.bottom
            horizontalCenter: parent.right
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
            left: parent.left
            right: parent.right
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
}
