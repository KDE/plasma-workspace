/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Controls 2.5 as QQC2

QQC2.TextField {
    property double backend

    maximumLength: 10
    horizontalAlignment: TextInput.AlignHCenter

    inputMethodHints: Qt.ImhFormattedNumbersOnly

    text: Math.round(backend * 100)/100;

    onBackendChanged: {
        text = Math.round(backend * 100)/100;
    }

    onTextChanged: {
        var textFloat = parseFloat(text);
        if (textFloat === undefined || isNaN(textFloat)) {
            return;
        }
        backend = textFloat;
    }

    onFocusChanged: {
        var textFloat = parseFloat(text);
        if (!focus && (textFloat === undefined || isNaN(textFloat))) {
            text = backend;
        }
    }
}
