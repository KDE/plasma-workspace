/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.2
import org.kde.kirigami 2.20 as Kirigami

SequentialAnimation {
    id: pulseAnimation
    objectName: "pulseAnimation"

    property Item targetItem
    readonly property int duration: Kirigami.Units.veryLongDuration * 5

    loops: Animation.Infinite
    alwaysRunToEnd: true

    ScaleAnimator {
        target: targetItem
        from: 1
        to: 1.2
        duration: pulseAnimation.duration * 0.15
        easing.type: Easing.InQuad
    }

    ScaleAnimator {
        target: targetItem
        from: 1.2
        to: 1
        duration: pulseAnimation.duration * 0.15
        easing.type: Easing.InQuad
    }

    PauseAnimation {
        duration: pulseAnimation.duration * 0.7
    }
}
