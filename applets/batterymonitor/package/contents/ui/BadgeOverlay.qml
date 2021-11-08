/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtGraphicalEffects 1.15

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore

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
        font.pixelSize: Math.max(icon.height / 4, PlasmaCore.Theme.smallestFont.pixelSize * 0.8)
    }

    layer.enabled: true
    layer.effect: DropShadow {
        horizontalOffset: 0
        verticalOffset: 0
        radius: PlasmaCore.Units.devicePixelRatio * 2
        samples: radius * 2
        color: Qt.rgba(0, 0, 0, 0.5)
    }
}
