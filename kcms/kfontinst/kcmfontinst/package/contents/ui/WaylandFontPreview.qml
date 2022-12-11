/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kirigami 2.20 as Kirigami

Column {
    id: waylandPreview

    Text {
        font.pointSize: 10
        text: waylandPreview.fontFamily || i18nc("@label", "ERROR: Could not determine font's name.")
    }

    Text {
        id: alphabet
        font.family: waylandPreview.fontFamily
        font.pointSize: waylandPreview.fontSize
        text: "abcdefghijklmnopqrstuvwxyz"
    }

    Text {
        font.family: waylandPreview.fontFamily
        font.pointSize: waylandPreview.fontSize
        text: alphabet.text.toUpperCase()
    }

    Text {
        font.family: waylandPreview.fontFamily
        font.pointSize: waylandPreview.fontSize
        text: "0123456789.:,;(*!?/)%-[]"
    }

    Repeater {
        model: [4, 8, 12, 16, 20, 32, 36, 48, 64]

        delegate: Text {
            font.family: waylandPreview.fontFamily
            font.pointSize: modelData
            text: preview.text || "The quick brown fox jumps over the lazy dog"
        }
    }
}
