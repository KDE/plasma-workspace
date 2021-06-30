/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Controls 2.5 as QQC2

QQC2.TextField {
    id: field

    property date backend
    horizontalAlignment: TextInput.AlignHCenter

    onBackendChanged: readIn()

    function getNormedDate() {
        var nD = new Date();
        nD.setHours(backend.getHours());
        nD.setMinutes(backend.getMinutes());
        return nD;
    }

    function readIn() {
        if (!backend) {
            return;
        }

        var hours = backend.getHours();
        var minutes = backend.getMinutes();
        if (hours < 10) {
            hours = "0" + hours;
        }
        if (minutes < 10) {
            minutes = "0" + minutes;
        }

        text = hours + ":" + minutes;
    }

    function submit() {
        if (text.length != 5) {
            return;
        }
        var hours = text.slice(0, 2);
        var minutes = text.slice(3, 5);

        var date = new Date();
        date.setHours(hours, minutes, 0, 0);

        backend = date;
    }

    onTextChanged: submit()
    inputMask: "00:00"
    selectByMouse: false
    inputMethodHints: Qt.ImhPreferNumbers
    validator: RegExpValidator { regExp: /^([0-1]?[0-9]|2[0-3]):[0-5][0-9]$/ }

    onEditingFinished: submit()
}
