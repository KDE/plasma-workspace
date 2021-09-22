/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.12

import org.kde.plasma.private.containmentlayoutmanager 1.0 as ContainmentLayoutManager 
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.14 as Kirigami

ContainmentLayoutManager.ResizeHandle {
    id: handle
    width: overlay.touchInteraction ? PlasmaCore.Units.gridUnit * 2 : PlasmaCore.Units.gridUnit
    height: width
    z: 999

    Kirigami.ShadowedRectangle {
        anchors.fill: parent
        color: resizeBlocked ? PlasmaCore.Theme.negativeTextColor : PlasmaCore.Theme.backgroundColor

        radius: width

        shadow.size: PlasmaCore.Units.smallSpacing
        shadow.color: Qt.rgba(0.0, 0.0, 0.0, 0.2)
        shadow.yOffset: PlasmaCore.Units.devicePixelRatio * 2

        border.width: PlasmaCore.Units.devicePixelRatio
        border.color: Qt.tint(Kirigami.Theme.textColor,
                            Qt.rgba(color.r, color.g, color.b, 0.3))
    }
    Rectangle {
        anchors {
            fill: parent
            margins: PlasmaCore.Units.devicePixelRatio / 2
        }
        border {
            width: PlasmaCore.Units.devicePixelRatio / 2
            color: Qt.rgba(1, 1, 1, 0.2)
        }
        gradient: Gradient {
            GradientStop { position: 0.0; color: handle.pressed ? Qt.rgba(0, 0, 0, 0.15) : Qt.rgba(1, 1, 1, 0.05) }
            GradientStop { position: 1.0; color: handle.pressed ? Qt.rgba(0, 0, 0, 0.15) : Qt.rgba(0, 0, 0, 0.05) }
        }

        radius: width
    }
    scale: overlay.open ? 1 : 0
    Behavior on scale {
        NumberAnimation {
            duration: PlasmaCore.Units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
}

