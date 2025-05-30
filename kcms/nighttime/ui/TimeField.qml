/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

RowLayout {
    id: root

    property string value: "00:00"

    signal activated(value: string)

    onValueChanged: refresh();

    QQC2.SpinBox {
        id: hoursSpinBox
        from: 0
        to: 23
        wrap: true
        onValueModified: notify();
    }

    QQC2.Label {
        text: i18nc("Time separator between hours and minutes", ":")
        textFormat: Text.PlainText
    }

    QQC2.SpinBox {
        id: minutesSpinBox
        from: 0
        to: 59
        wrap: true
        onValueModified: notify();
    }

    function notify(): void {
        const date = new Date();
        date.setHours(hoursSpinBox.value, minutesSpinBox.value, 0, 0);
        activated(date.toLocaleString(Qt.locale("C"), "hh:mm"));
    }

    function refresh(): void {
        const date = Date.fromLocaleString(Qt.locale("C"), value, "hh:mm")
        hoursSpinBox.value = date.getHours();
        minutesSpinBox.value = date.getMinutes();
    }

    Component.onCompleted: refresh();
}
