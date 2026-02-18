/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

import QtQuick
import QtQuick.Controls as QQC
import org.kde.plasma.private.keyboardindicator as KI

QQC.Label {
    text: `${keyState.pressedCount}-${keyState.releasedCount}`
    KI.KeyState {
        id: keyState
        property int pressedCount: 0
        property int releasedCount: 0
        key: Qt.Key_Control
        onPressedChanged: if (pressed) {
            pressedCount += 1;
        } else {
            releasedCount += 1;
        }
    }
}
