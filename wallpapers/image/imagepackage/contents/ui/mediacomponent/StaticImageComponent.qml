/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import org.kde.plasma.wallpapers.image as Wallpaper

BaseMediaComponent {
    id: staticImageComponent

    readonly property alias status: mainImage.status

    blurSource: blurLoader.item

    Wallpaper.TransientImage {
        id: mainImage
        anchors.fill: parent

        fillMode: staticImageComponent.fillMode
        source: staticImageComponent.source
    }

    Loader {
        id: blurLoader
        anchors.fill: parent
        z: 0
        active: staticImageComponent.blurEnabled
        sourceComponent: Image {
            asynchronous: true
            cache: false
            autoTransform: true
            fillMode: Image.PreserveAspectCrop
            source: mainImage.source
            sourceSize: mainImage.sourceSize
            visible: false // will be rendered by the blur
        }
    }
}
