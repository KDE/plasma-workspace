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


ContainmentLayoutManager.ResizeHandle {
    width: overlay.touchInteraction ? units.gridUnit * 2 : units.gridUnit
    height: width
    z: 999
    Rectangle {
        color: resizeBlocked ? theme.negativeTextColor : theme.backgroundColor
        anchors.fill: parent
        radius: width
        opacity: 0.6
    }
    scale: overlay.open ? 1 : 0
    Behavior on scale {
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
}

