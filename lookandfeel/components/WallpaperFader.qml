/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.private.sessions 2.0
import org.kde.breeze.components

Item {
    id: wallpaperFader
    property alias source: wallpaperBlur.source
    property real factor: 1
    // Use this to control the opacity of UI elements so that they fade in/out
    property real uiOpacity: 1
    readonly property bool lightColorScheme: Math.max(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b) > 0.5

    state: "on"

    FastBlur {
        id: wallpaperBlur
        anchors.fill: parent
        radius: 50 * wallpaperFader.factor
    }
    ShaderEffect {
        id: wallpaperShader
        anchors.fill: parent
        supportsAtlasTextures: true
        property var source: ShaderEffectSource {
            sourceItem: wallpaperBlur
            live: true
            hideSource: true
            textureMirroring: ShaderEffectSource.NoMirroring
        }

        readonly property real contrast: 0.65 * wallpaperFader.factor + (1 - wallpaperFader.factor)
        readonly property real saturation: 1.6 * wallpaperFader.factor + (1 - wallpaperFader.factor)
        readonly property real intensity: (wallpaperFader.lightColorScheme ? 1.7 : 0.6) * wallpaperFader.factor + (1 - wallpaperFader.factor)

        readonly property real transl: (1.0 - contrast) / 2.0;
        readonly property real rval: (1.0 - saturation) * 0.2126;
        readonly property real gval: (1.0 - saturation) * 0.7152;
        readonly property real bval: (1.0 - saturation) * 0.0722;

        property var colorMatrix: Qt.matrix4x4(
            contrast, 0,        0,        0.0,
            0,        contrast, 0,        0.0,
            0,        0,        contrast, 0.0,
            transl,   transl,   transl,   1.0).times(Qt.matrix4x4(
                rval + saturation, rval,     rval,     0.0,
                gval,     gval + saturation, gval,     0.0,
                bval,     bval,     bval + saturation, 0.0,
                0,        0,        0,        1.0)).times(Qt.matrix4x4(
                    intensity, 0,         0,         0,
                    0,         intensity, 0,         0,
                    0,         0,         intensity, 0,
                    0,         0,         0,         1
                ));

        fragmentShader: "qrc:/qt/qml/org/kde/breeze/components/shaders/WallpaperFader.frag.qsb"
    }

    states: [
        State {
            name: "on"
            PropertyChanges {
                target: wallpaperFader
                factor: 1
                uiOpacity: 1
            }
        },
        State {
            name: "off"
            PropertyChanges {
                target: wallpaperFader
                factor: 0
                uiOpacity: 0
            }
        }
    ]
    transitions: Transition {
        NumberAnimation {
            properties: "factor,uiOpacity"
            duration: Kirigami.Units.veryLongDuration * 2
            easing.type: Easing.InOutQuad
        }
    }
}
