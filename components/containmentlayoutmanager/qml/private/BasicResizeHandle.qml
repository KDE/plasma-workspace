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
        shadow.yOffset: Kirigami.Units.devicePixelRatio * 2

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

