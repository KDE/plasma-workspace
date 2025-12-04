/*
    SPDX-FileCopyrightText: 2022 Janet Blackquill <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import org.kde.kirigami as Kirigami

Timer {
    property bool isTriggered: false

    function reset() {
        isTriggered = false;
        restart();
    }

    interval: Kirigami.Units.humanMoment

    onTriggered: {
        isTriggered = true;
    }
}
