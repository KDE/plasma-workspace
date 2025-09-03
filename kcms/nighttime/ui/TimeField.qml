/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

RowLayout {
    id: root

    readonly property var locale: Qt.locale()
    readonly property bool isAmPm: locale.timeFormat().toUpperCase().includes("AP")
    property date value: new Date()

    signal activated(value: date)

    onValueChanged: refresh();

    QQC2.SpinBox {
        id: hoursSpinBox
        from: isAmPm ? 1 : 0
        to: isAmPm ? 12 : 23
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

    QQC2.SpinBox {
        id: amPmSpinBox

        readonly property list<string> items: [root.locale.amText, root.locale.pmText]

        visible: isAmPm
        from: 0
        to: items.length - 1
        wrap: true
        validator: RegularExpressionValidator {
            regularExpression: new RegExp("(" + amPmSpinBox.items.map(meridiem => kcm.escapeRegExp(meridiem)).join("|") + ")", "i")
        }
        textFromValue: function(value, locale) {
            return items[value];
        }
        valueFromText: function(text, locale) {
            const upperText = text.toUpperCase();
            for (const [index, value] of items.entries()) {
                if (value.toUpperCase() === upperText) {
                    return index;
                }
            }
            return value;
        }
        onValueModified: notify();
    }

    function notify(): void {
        let hours = hoursSpinBox.value;
        if (isAmPm) {
            if (hours === 12) {
                hours = 0;
            }
            if (amPmSpinBox.value === 1) { // pm
                hours += 12;
            }
        }

        const date = new Date();
        date.setHours(hours, minutesSpinBox.value, 0, 0);
        activated(date);
    }

    function refresh(): void {
        let hours = value.getHours();
        if (isAmPm) {
            hours %= 12;
            if (hours === 0) {
                hours = 12;
            }
        }

        hoursSpinBox.value = hours;
        minutesSpinBox.value = value.getMinutes();
        amPmSpinBox.value = value.getHours() >= 12;
    }

    Component.onCompleted: refresh();
}
