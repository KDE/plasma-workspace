/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

import QtQuick
import org.kde.plasma.private.keyboardindicator as KI

Item {
    width: 100
    height: 100
    property alias keyState: keyState
    KI.KeyState {
        id: keyState
        key: Qt.Key_CapsLock
    }
}
