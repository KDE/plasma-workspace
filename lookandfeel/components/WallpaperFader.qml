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
    property Item clock
    property Item mainStack
    property Item footer
    property alias source: wallpaperBlur.source
    property real factor: 0
    readonly property bool lightColorScheme: Math.max(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b) > 0.5

    property bool alwaysShowClock: false

    state: "on"

    Behavior on factor {
        NumberAnimation {
            target: wallpaperFader
            property: "factor"
            duration: Kirigami.Units.veryLongDuration * 2
            easing.type: Easing.InOutQuad
        }
    }
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

        readonly property real contrast: 0.8 * wallpaperFader.factor + (1 - wallpaperFader.factor)
        readonly property real saturation: 1.5 * wallpaperFader.factor + (1 - wallpaperFader.factor)
        readonly property real intensity: (wallpaperFader.lightColorScheme ? 1.6 : 0.7) * wallpaperFader.factor + (1 - wallpaperFader.factor)

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
                target: mainStack
                opacity: 1
            }
            PropertyChanges {
                target: footer
                opacity: 1
            }
            PropertyChanges {
                target: wallpaperFader
                factor: 1
            }
            PropertyChanges {
                target: clock.shadow
                opacity: 0
            }
            PropertyChanges {
                target: clock
                opacity: 1
            }
        },
        State {
            name: "off"
            PropertyChanges {
                target: mainStack
                opacity: 0
            }
            PropertyChanges {
                target: footer
                opacity: 0
            }
            PropertyChanges {
                target: wallpaperFader
                factor: 0
            }
            PropertyChanges {
                target: clock.shadow
                opacity: wallpaperFader.alwaysShowClock ? 1 : 0
            }
            PropertyChanges {
                target: clock
                opacity: wallpaperFader.alwaysShowClock ? 1 : 0
            }
        }
    ]
    transitions: [
        Transition {
            from: "off"
            to: "on"
            //Note: can't use animators as they don't play well with parallelanimations
            NumberAnimation {
                targets: [mainStack, footer, clock]
                property: "opacity"
                duration: Kirigami.Units.veryLongDuration
                easing.type: Easing.InOutQuad
            }
        },
        Transition {
            from: "on"
            to: "off"
            NumberAnimation {
                targets: [mainStack, footer, clock]
                property: "opacity"
                duration: Kirigami.Units.veryLongDuration
                easing.type: Easing.InOutQuad
            }
        }
    ]
}
