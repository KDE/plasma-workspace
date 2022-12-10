/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kirigami 2.20 as Kirigami

Column {
    id: waylandPreview

    spacing: Kirigami.Units.smallSpacing

    readonly property string fontFamily: sourceLoader.status === FontLoader.Ready ? sourceLoader.name : preview.name
    readonly property bool atMax: fontSize === 128
    readonly property bool atMin: fontSize === 2
    property real fontSize: 14
    property string name
    property int face: 0

    function zoomIn() {
        fontSize = Math.min(128, fontSize + 2);
    }

    function zoomOut() {
        fontSize = Math.max(2, fontSize - 2);
    }

    FontLoader {
        id: sourceLoader
        source: preview.name
    }

    WheelHandler {
        property int angleDelta: 0
        onWheel: {
            angleDelta += event.angleDelta.y;
            if (angleDelta >= 120) {
                angleDelta = 0;
                waylandPreview.zoomIn();
            } else if (angleDelta <= -120) {
                angleDelta = 0;
                waylandPreview.zoomOut();
            }
        }
    }

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
