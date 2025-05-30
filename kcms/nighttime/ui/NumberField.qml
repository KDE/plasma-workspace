/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2

QQC2.TextField {
    id: root

    property double value: 0.0
    property alias from: validator.bottom
    property alias to: validator.top

    signal valueEdited(value: double)

    horizontalAlignment: TextInput.AlignHCenter
    verticalAlignment: TextInput.AlignVCenter

    validator: DoubleValidator {
        id: validator
    }

    onFocusChanged: {
        if (!focus) {
            apply(value);
        }
    }

    onValueChanged: {
        if (!focus) {
            apply(value);
        }
    }

    onAccepted: {
        apply(value);
    }

    onTextEdited: {
        if (!text) {
            valueEdited(0);
        } else {
            valueEdited(Number.fromLocaleString(Qt.locale(), text));
        }
    }

    function apply(value: double): void {
        text = value.toLocaleString(Qt.locale());
    }

    Component.onCompleted: apply(value);
}
