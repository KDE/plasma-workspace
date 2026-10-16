/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kirigami as Kirigami

SequentialAnimation {
    id: pulseAnimation
    objectName: "pulseAnimation"

    property Item targetItem
    readonly property int duration: Kirigami.Units.veryLongDuration * 5

    loops: Animation.Infinite
    alwaysRunToEnd: true
    // Avoiding an animator due to https://qt-project.atlassian.net/browse/QTBUG-150367
    NumberAnimation {
        target: pulseAnimation.targetItem
        property: "scale"
        from: 1
        to: 1.2
        duration: pulseAnimation.duration * 0.15
        easing.type: Easing.InQuad
    }

    NumberAnimation {
        target: pulseAnimation.targetItem
        property: "scale"
        from: 1.2
        to: 1
        duration: pulseAnimation.duration * 0.15
        easing.type: Easing.InQuad
    }

    PauseAnimation {
        duration: pulseAnimation.duration * 0.7
    }
}
