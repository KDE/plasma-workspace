/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import Qt5Compat.GraphicalEffects

import org.kde.kirigami as Kirigami

Item {
    id: greeterFader

    required property Item mainStack
    required property Item footer
    required property Item clock

    required property bool alwaysShowClock

    state: "off"

    states: [
        State {
            name: "on"
            PropertyChanges {
                mainStack.opacity: 1
                footer.opacity: 1
                clock.opacity: 1
                // Intentional: shadow is not needed due to
                // WallpaperFader providing enhanced contrast
                clock.shadow.opacity: 0
            }
        },
        State {
            name: "off"
            PropertyChanges {
                mainStack.opacity: 0
                footer.opacity: 0
                clock.opacity: greeterFader.alwaysShowClock ? 1 : 0
                clock.shadow.opacity: greeterFader.alwaysShowClock ? 1 : 0
            }
        }
    ]

    transitions: [
        Transition {
            // NOTE: can't use animators as they don't play well with parallelanimations
            NumberAnimation {
                targets: [greeterFader.mainStack, greeterFader.footer, greeterFader.clock]
                property: "opacity"
                duration: Kirigami.Units.veryLongDuration
                easing.type: Easing.InOutQuad
            }
        },
        Transition {
            NumberAnimation {
                targets: [greeterFader.clock.shadow]
                property: "opacity"
                // Intentional: matches WallpaperFader animation because we only
                // want shadow for enhanced contrast when wallpaper unfaded
                duration: Kirigami.Units.veryLongDuration * 2
                easing.type: Easing.InOutQuad
            }
        }
    ]
}
