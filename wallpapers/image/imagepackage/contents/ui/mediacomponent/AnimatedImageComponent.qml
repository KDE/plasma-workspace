/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

import org.kde.plasma.wallpapers.image as PlasmaWallpaper

import org.kde.kwindowsystem

BaseMediaComponent {
    id: animatedImageComponent

    readonly property rect desktopRect: Window.window ? Qt.rect(Window.window.x, Window.window.y, Window.window.width, Window.window.height) : Qt.rect(0, 0, 0, 0)
    readonly property alias status: mainImage.status

    blurSource: blurLoader.item

    PlasmaWallpaper.MaximizedWindowMonitor {
        id: activeWindowMonitor
        regionGeometry: animatedImageComponent.desktopRect
    }

    AnimatedImage {
        id: mainImage
        anchors.fill: parent
        asynchronous: true
        cache: false
        autoTransform: true

        fillMode: animatedImageComponent.fillMode
        source: animatedImageComponent.source
        // sourceSize is read-only
        // https://github.com/qt/qtdeclarative/blob/23b4ab24007f489ac7c2b9ceabe72fa625a51f3d/src/quick/items/qquickanimatedimage_p.h#L39

        paused: activeWindowMonitor.count > 0 && !KWindowSystem.showingDesktop
    }

    Loader {
        id: blurLoader
        anchors.fill: parent
        active: animatedImageComponent.blurEnabled
        sourceComponent: Image {
            asynchronous: true
            cache: false
            autoTransform: true
            fillMode: Image.PreserveAspectCrop
            source: mainImage.source
            sourceSize: animatedImageComponent.sourceSize
            visible: false // will be rendered by the blur
        }
    }
}
