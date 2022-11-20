/*
    SPDX-FileCopyrightText: 2022 Janet Blackquill <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import org.kde.kirigami 2.16 as Kirigami

Timer {
    property bool isTriggered: false
    interval: Kirigami.Units.humanMoment
    function reset() {
        isTriggered = false
        restart()
    }
    onTriggered: isTriggered = true
}
