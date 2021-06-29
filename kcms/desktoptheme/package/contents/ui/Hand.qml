/*
    SPDX-FileCopyrightText: 2012 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0

import org.kde.plasma.core 2.0 as PlasmaCore

PlasmaCore.SvgItem {
    id: handRoot

    property alias rotation: rotation.angle
    property double svgScale
    property double horizontalRotationOffset: 0
    property double verticalRotationOffset: 0
    property string rotationCenterHintId
    readonly property double horizontalRotationCenter: {
        if (svg.hasElement(rotationCenterHintId)) {
            var hintedCenterRect = svg.elementRect(rotationCenterHintId),
                handRect = svg.elementRect(elementId),
                hintedX = hintedCenterRect.x - handRect.x + hintedCenterRect.width/2;
            return Math.round(hintedX * svgScale) + Math.round(hintedX * svgScale) % 2;
        }
        return width/2;
    }
    readonly property double verticalRotationCenter: {
        if (svg.hasElement(rotationCenterHintId)) {
            var hintedCenterRect = svg.elementRect(rotationCenterHintId),
                handRect = svg.elementRect(elementId),
                hintedY = hintedCenterRect.y - handRect.y + hintedCenterRect.height/2;
            return Math.round(hintedY * svgScale) + width % 2;
        }
        return width/2;
    }

    width: Math.round(naturalSize.width * svgScale) + Math.round(naturalSize.width * svgScale) % 2
    height: Math.round(naturalSize.height * svgScale) + width % 2
    anchors {
        top: clock.verticalCenter
        topMargin: -verticalRotationCenter + verticalRotationOffset
        left: clock.horizontalCenter
        leftMargin: -horizontalRotationCenter + horizontalRotationOffset
    }

    svg: clockSvg
    transform: Rotation {
        id: rotation
        angle: 0
        origin {
            x: handRoot.horizontalRotationCenter
            y: handRoot.verticalRotationCenter
        }
        Behavior on angle {
            RotationAnimation {
                id: anim
                duration: PlasmaCore.Units.longDuration
                direction: RotationAnimation.Clockwise
                easing.type: Easing.OutElastic
                easing.overshoot: 0.5
            }
        }
    }
}
