/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

Rectangle {
    id: backgroundColor

    color: "black"
    z: -2

    property bool blur: false
    required property url source
    property int fillMode
    required property size sourceSize

    /**
     * This defines the item that will be blurred and used in the background
     */
    property var blurSource
    readonly property bool blurEnabled: backgroundColor.blur
        && (backgroundColor.fillMode === Image.PreserveAspectFit || backgroundColor.fillMode === Image.Pad)

    /**
     * This defines the background item that will be cropped when
     * "Span Screens" is enabled.
     *
     * @since 5.26
     */
    property var cropSource
    property bool spanScreens: false
    property rect targetRect

    layer.enabled: StackView.status !== StackView.Active && StackView.status !== StackView.Deactivating

    Loader {
        anchors.fill: parent
        active: blurEnabled
        visible: active
        z: 0
        sourceComponent: GaussianBlur {
            source: backgroundColor.blurSource
            radius: 32
            samples: 65
        }
    }

    // Crop background when span screens is enabled
    Loader {
        active: backgroundColor.spanScreens
        anchors.fill: parent
        asynchronous: true
        visible: active
        z: 0
        sourceComponent: ShaderEffectSource {
            hideSource: true
            sourceItem: backgroundColor.cropSource
            sourceRect: backgroundColor.targetRect
        }
    }
}
