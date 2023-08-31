/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

BaseMediaComponent {
    id: staticImageComponent

    readonly property alias status: mainImage.status

    blurSource: blurLoader.item

    Image {
        id: mainImage
        anchors.fill: parent
        asynchronous: true
        cache: false
        autoTransform: true

        fillMode: staticImageComponent.fillMode
        source: staticImageComponent.source
        sourceSize: staticImageComponent.sourceSize
    }

    Loader {
        id: blurLoader
        anchors.fill: parent
        z: 0
        active: blurEnabled
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
