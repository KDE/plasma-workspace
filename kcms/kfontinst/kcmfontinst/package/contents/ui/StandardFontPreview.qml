/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15

import org.kde.kirigami 2.20 as Kirigami

Column {
    id: standardPreview

    height: implicitHeight

    Text {
        id: alphabet
        font.family: preview.fontFamily
        font.pointSize: preview.fontSize
        font.styleName: preview.styleName
        text: "abcdefghijklmnopqrstuvwxyz"
    }

    Text {
        font.family: preview.fontFamily
        font.pointSize: preview.fontSize
        font.styleName: preview.styleName
        text: alphabet.text.toUpperCase()
    }

    Text {
        font.family: preview.fontFamily
        font.pointSize: preview.fontSize
        font.styleName: preview.styleName
        text: "0123456789.:,;(*!?/)%-[]"
    }

    Kirigami.Separator {
        width: parent.width
    }

    Repeater {
        model: [8, 12, 16, 20, 24, 32, 36, 48, 64]

        delegate: Text {
            font.family: preview.fontFamily
            font.pointSize: modelData
            font.styleName: preview.styleName
            text: preview.text || "The quick brown fox jumps over the lazy dog"
        }
    }
}
