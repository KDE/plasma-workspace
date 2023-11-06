/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2

QQC2.TextField {
    property string backend

    horizontalAlignment: TextInput.AlignHCenter

    function getNormedDate() {
        var nD = new Date();
        var d = backendToDate();
        nD.setHours(d.getHours());
        nD.setMinutes(d.getMinutes());
        return nD;
    }

    function updateTextFromBackend() {
        if (!backend || backend.length !== 4) {
            return;
        }
        var hours = backend.slice(0, 2);
        var minutes = backend.slice(2, 4);
        text = hours + ":" + minutes;
    }

    function backendToDate() {
        if (backend.length !== 4) {
            return;
        }
        var hours = backend.slice(0, 2);
        var minutes = backend.slice(2, 4);
        var date = new Date();
        date.setHours(hours, minutes, 0, 0);
        return date
    }

    function updateBackendFromText() {
        if (text.length !== 5) {
            return;
        }
        var hours = text.slice(0, 2);
        var minutes = text.slice(3, 5);
        backend = hours + minutes;
    }

    inputMask: "00:00"
    selectByMouse: false
    inputMethodHints: Qt.ImhTime
    validator: RegularExpressionValidator { regularExpression: /^[0-2]?[0-9]:[0-5][0-9]$/ }

    onTextEdited: updateBackendFromText()
    onEditingFinished: updateBackendFromText()

    onBackendChanged: updateTextFromBackend()
}
