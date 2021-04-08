/***************************************************************************
 *   Copyright (C) 2016 Kai Uwe Broulik <kde@privat.broulik.de>            *
 *   Copyright (C) 2016 Marco Martin <mart@kde.org>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.4
import QtGraphicalEffects 1.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

Rectangle {
    property alias text: label.text
    property Item icon

    color: PlasmaCore.ColorScope.backgroundColor
    width: Math.max(PlasmaCore.Units.gridUnit, label.width + PlasmaCore.Units.devicePixelRatio * 2)
    height: label.height
    radius: PlasmaCore.Units.devicePixelRatio * 3
    opacity: 0.9

    PlasmaComponents3.Label {
        id: label
        anchors.centerIn: parent
        font.pixelSize: Math.max(icon.height/4, PlasmaCore.Theme.smallestFont.pixelSize*0.8)
    }

    layer.enabled: true
    layer.effect: DropShadow {
        horizontalOffset: 0
        verticalOffset: 0
        radius: PlasmaCore.Units.devicePixelRatio * 2
        samples: radius*2
        color: Qt.rgba(0, 0, 0, 0.5)
    }
}

