/*
    SPDX-FileCopyrightText: 2016 David Rosca <nowrep@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.5
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.5 as Kirigami

Kirigami.FormLayout {
    property alias cfg_volumeStep: volumeStep.value

    QQC2.SpinBox {
        id: volumeStep

        // So it doesn't resize itself when showing a 2 or 3-digit number
        Layout.minimumWidth: Kirigami.Units.gridUnit * 3

        Kirigami.FormData.label: i18n("Volume step:")

        from: 1
        to: 100
        stepSize: 1
        editable: true
        textFromValue: function(value) {
            return value + "%";
        }
        valueFromText: function(text) {
            return parseInt(text);
        }
    }
}
