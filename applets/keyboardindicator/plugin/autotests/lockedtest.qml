/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

import QtQuick.Controls as QQC
import org.kde.plasma.private.keyboardindicator as KI

QQC.Label {
    text: capsLockState.locked ? "locked" : "unlocked"
    KI.KeyState {
        id: capsLockState
        key: Qt.Key_CapsLock
    }
}
