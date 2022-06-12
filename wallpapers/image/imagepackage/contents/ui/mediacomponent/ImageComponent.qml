/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtGraphicalEffects 1.15

Rectangle {
    id: backgroundColor

    color: "black"
    z: -2

    property bool blur: false
    property alias mainImage: mainImage
    property alias source: mainImage.source
    property alias fillMode: mainImage.fillMode
    property alias sourceSize: mainImage.sourceSize
    property alias status: mainImage.status

    Image {
        id: mainImage
        anchors.fill: parent

        asynchronous: true
        cache: false
        autoTransform: true
        z: 0
    }

    Loader {
        id: blurLoader
        anchors.fill: parent
        z: -1
        active: backgroundColor.blur && (mainImage.fillMode === Image.PreserveAspectFit || mainImage.fillMode === Image.Pad)
        visible: active
        sourceComponent: Item {
            Image {
                id: blurSource
                anchors.fill: parent
                asynchronous: true
                cache: false
                autoTransform: true
                fillMode: Image.PreserveAspectCrop
                source: mainImage.source
                sourceSize: mainImage.sourceSize
                visible: false // will be rendered by the blur
            }

            GaussianBlur {
                id: blurEffect
                anchors.fill: parent
                source: blurSource
                radius: 32
                samples: 65
                visible: blurSource.status === Image.Ready
            }
        }
    }
}
