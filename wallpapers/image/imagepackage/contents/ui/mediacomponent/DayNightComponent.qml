/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import org.kde.plasma.wallpapers.image

BaseMediaComponent {
    id: dayNightComponent
    blurSource: blurLoader.item

    readonly property alias status: dayNightView.status

    DayNightView {
        id: dayNightView
        anchors.fill: parent
        fillMode: dayNightComponent.fillMode
        snapshot: dayNightWallpaper.snapshot
    }

    DayNightWallpaper {
        id: dayNightWallpaper
        initialState: configuration.DarkLightScheduleState
        source: dayNightComponent.source

        onStateChanged: () => {
            if (configuration.DarkLightScheduleState != state) {
                configuration.DarkLightScheduleState = state;
                configuration.writeConfig();
            }
        }
    }

    Loader {
        id: blurLoader
        anchors.fill: parent
        active: dayNightComponent.blurEnabled
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
