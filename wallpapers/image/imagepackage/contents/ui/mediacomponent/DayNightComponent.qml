/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtPositioning
import QtQuick
import QtQuick.Controls
import org.kde.plasma.wallpapers.image

Rectangle {
    id: root

    color: "black"
    z: -2

    property bool blur: false
    property alias source: dayNightWallpaper.source
    property alias fillMode: dayNightImage.fillMode
    property alias sourceSize: dayNightImage.sourceSize
    readonly property alias status: dayNightImage.status

    layer.enabled: StackView.status !== StackView.Active && StackView.status !== StackView.Deactivating

    PositionSource {
        id: locationSource
        active: configuration.AllowGeolocation
    }

    DoubleBufferedImage {
        id: dayNightImage
        anchors.fill: parent
        bottomUrl: dayNightWallpaper.current
        topUrl: dayNightWallpaper.next
        blendFactor: dayNightWallpaper.blendFactor
    }

    DayNightWallpaper {
        id: dayNightWallpaper
        location: locationSource.active ? locationSource.position.coordinate : undefined
    }
}
