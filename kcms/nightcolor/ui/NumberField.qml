/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2024 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2

QQC2.TextField {
    property double backend

    maximumLength: 10
    horizontalAlignment: TextInput.AlignHCenter

    inputMethodHints: Qt.ImhFormattedNumbersOnly

    text: (Math.round(backend * 100) / 100).toLocaleString(Qt.locale());

    // Only update the value when accepting, and the validator agrees
    onEditingFinished: {
        backend = Number.fromLocaleString(Qt.locale(), text);
    }

    // If the validator is not happy, reset the text to the last good value
    onFocusChanged: {
        if (!focus && !acceptableInput) {
            backendChanged();
        }
    }
}
