/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Window 2.15

import org.kde.plasma.wallpapers.image 2.0 as PlasmaWallpaper

import org.kde.kwindowsystem 1.0

BaseMediaComponent {
    id: animatedImageComponent

    readonly property rect desktopRect: Qt.rect(Window.window.x, Window.window.y, Window.window.width, Window.window.height)
    readonly property alias status: mainImage.status

    blurSource: blurLoader.item

    KWindowSystem {
        id: kwindowsystem
    }

    PlasmaWallpaper.MaximizedWindowMonitor {
        id: activeWindowMonitor
        targetRect: animatedImageComponent.desktopRect
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

        paused: (activeWindowMonitor.count > 0 && !kwindowsystem.showingDesktop)
             || kwindowsystem.isPlatformX11 // xcb_glx leaks memory
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
