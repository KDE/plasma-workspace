/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    property string backend: "0000"

    spacing: Kirigami.Units.smallSpacing

    component TimePartSpinBox : QQC2.SpinBox {
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered || activeFocus

        editable: true

        textFromValue: function(value, locale) {
            return value.toString().padStart(2, "0");
        }

        valueFromText: function(text, locale) {
            return parseInt(text);
        }

        onValueModified: {
            root.updateBackendFromFields();
        }
    }

    TimePartSpinBox {
        id: hoursField

        QQC2.ToolTip.text: i18nc("@info:tooltip Part of a control for setting a time", "hour")

        from: 0
        to: 23
    }

    QQC2.Label {
        text: i18nc("Time separator between hours and minutes", ":")
    }

    TimePartSpinBox {
        id: minutesField

        QQC2.ToolTip.text: i18nc("@info:tooltip Part of a control for setting a time", "minute")

        from: 0
        to: 59
        stepSize: 5
    }

    function updateFieldsFromBackend(): void {
        if (!backend || backend.length !== 4) {
            return;
        }
        hoursField.value = hoursField.valueFromText(backend.slice(0, 2));
        minutesField.value = minutesField.valueFromText(backend.slice(2, 4));
    }

    function updateBackendFromFields(): void {
        backend = hoursField.textFromValue(hoursField.value) + minutesField.textFromValue(minutesField.value);
    }

    Component.onCompleted: {
        updateFieldsFromBackend();
    }

    function backendToDate(): date {
        if (!backend || backend.length !== 4) {
            return;
        }
        const hours = backend.slice(0, 2);
        const minutes = backend.slice(2, 4);
        const date = new Date();
        date.setHours(hours, minutes, 0, 0);
        return date;
    }

    function getNormedDate(): date {
        const normedDate = new Date();
        const backendDate = backendToDate();
        normedDate.setHours(backendDate.getHours());
        normedDate.setMinutes(backendDate.getMinutes());
        return normedDate;
    }
}
