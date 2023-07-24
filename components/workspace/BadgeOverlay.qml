/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import Qt5Compat.GraphicalEffects

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

Rectangle {
    property alias text: label.text
    property Item icon

    color: Kirigami.Theme.backgroundColor
    width: Math.max(Kirigami.Units.gridUnit, label.width + 2)
    height: label.height
    radius: 3
    opacity: 0.9

    PlasmaComponents3.Label {
        id: label
        anchors.centerIn: parent
        font.pixelSize: Math.max(icon.height / 4, Kirigami.Theme.smallFont.pixelSize * 0.8)
    }

    layer.enabled: true
    layer.effect: DropShadow {
        horizontalOffset: 0
        verticalOffset: 0
        radius: 2
        samples: radius * 2
        color: Qt.rgba(0, 0, 0, 0.5)
    }
}
