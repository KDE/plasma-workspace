/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import Qt5Compat.GraphicalEffects

import org.kde.kirigami as Kirigami

Item {
    id: wallpaperFader

    required property real factor
    property alias source: wallpaperBlur.source

    /* These values match those used in the Plasma overview effect */
    readonly property int blurRadius: 64
    readonly property real underlayOpacity: 0.7

    Behavior on factor {
        NumberAnimation {
            target: wallpaperFader
            property: "factor"
            duration: Kirigami.Units.veryLongDuration * 2
            easing.type: Easing.InOutQuad
        }
    }

    FastBlur {
        id: wallpaperBlur
        anchors.fill: parent
        radius: wallpaperFader.blurRadius * wallpaperFader.factor
    }

    Rectangle {
        anchors.fill: parent

        opacity: factor * wallpaperFader.underlayOpacity
        color: Kirigami.Theme.backgroundColor
    }
}
