/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtPositioning
import QtQuick
import QtQuick.Controls
import org.kde.plasma.wallpapers.image

BaseMediaComponent {
    id: dayNightComponent
    blurSource: blurLoader.item

    readonly property alias status: dayNightView.status

    PositionSource {
        id: locationSource
        active: configuration.Geolocation
    }

    DayNightView {
        id: dayNightView
        anchors.fill: parent
        fillMode: dayNightComponent.fillMode
        sourceSize: dayNightComponent.sourceSize
        snapshot: dayNightWallpaper.snapshot
    }

    DayNightWallpaper {
        id: dayNightWallpaper
        location: locationSource.active ? locationSource.position.coordinate : undefined
        source: dayNightComponent.source
    }

    Loader {
        id: blurLoader
        anchors.fill: parent
        active: blurEnabled
        sourceComponent: Image {
            asynchronous: true
            cache: false
            autoTransform: true
            fillMode: Image.PreserveAspectCrop
            source: dayNightWallpaper.snapshot.bottom
            sourceSize: dayNightComponent.sourceSize
            visible: false
        }
    }
}
