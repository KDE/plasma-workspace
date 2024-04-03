/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.5 as QQC2
import QtQuick.Layouts as QQL2

import org.kde.kirigami as Kirigami

QQL2.RowLayout {
    property string backend

    QQC2.SpinBox {
        id: hoursField

        QQC2.ToolTip.text: i18nc("@info:tooltip Part of a control for setting a time", "hour")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay 
        QQC2.ToolTip.visible: hovered || activeFocus
        
        from: 0
        to: 23
        editable: true

        onValueModified: {
            updateBackendFromFields()
        }
        textFromValue: function(value, locale) {
            return value.toString().padStart(2, "0");
        }
        valueFromText: function(text, locale) {
            return parseInt(text);
        }
    }

    QQC2.Label {
        text: i18nc("Time separator between hours and minutes", ":")
    }

    QQC2.SpinBox {
        id: minutesField

        QQC2.ToolTip.text: i18nc("@info:tooltip Part of a control for setting a time", "minute")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay 
        QQC2.ToolTip.visible: hovered || activeFocus
        
        from: 0
        to: 59
        stepSize: 5
        editable: true

        onValueModified: {
            updateBackendFromFields()
        }
        textFromValue: function(value, locale) {
            return value.toString().padStart(2, "0");
        }
        valueFromText: function(text, locale) {
            return parseInt(text);
        }
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

    function backendToDate() {
        if (!backend || backend.length !== 4) {
            return;
        }
        var hours = backend.slice(0, 2);
        var minutes = backend.slice(2, 4);
        var date = new Date();
        date.setHours(hours, minutes, 0, 0);
        return date;
        return date;
    }

    function getNormedDate(): void {
        var nD = new Date();
        var d = backendToDate();
        nD.setHours(d.getHours());
        nD.setMinutes(d.getMinutes());
        return nD;
    }
}
