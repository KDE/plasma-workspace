/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtGraphicalEffects 1.15

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore

Item {
    property alias text: label.text
    property Item icon

    readonly property bool minimalSize: icon.height < PlasmaCore.Units.iconSizes.medium

    width: Math.max(PlasmaCore.Units.gridUnit, label.width + PlasmaCore.Units.devicePixelRatio * 2)
    height: label.height

    Rectangle {
        anchors.fill: parent

        color: PlasmaCore.ColorScope.backgroundColor
        radius: PlasmaCore.Units.devicePixelRatio * 3
        opacity: minimalSize ? 0.6 : 0.9

        layer.enabled: !minimalSize
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 0
            radius: PlasmaCore.Units.devicePixelRatio * 2
            samples: radius * 2
            color: Qt.rgba(0, 0, 0, 0.5)
        }
    }

    PlasmaComponents3.Label {
        id: label
        anchors.centerIn: parent
        font.pixelSize: Math.max(icon.height / 4, PlasmaCore.Theme.smallestFont.pixelSize * 0.8)
        style: minimalSize ? Text.Outline : Text.Normal
        styleColor: PlasmaCore.ColorScope.backgroundColor
    }
}
