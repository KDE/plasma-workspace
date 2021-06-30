/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import org.kde.kirigami 2.5 as Kirigami
import QtQuick.Controls 2.5 as QQC2

Kirigami.FormLayout {
    twinFormLayouts: parentLayout

    property double latitude
    property double longitude

    property var morningTimings: sunCalc.getMorningTimings(latitude, longitude)
    property var eveningTimings: sunCalc.getEveningTimings(latitude, longitude)

    function reset() {
        morningTimings = sunCalc.getMorningTimings(latitude, longitude);
        eveningTimings = sunCalc.getEveningTimings(latitude, longitude);
    }

    function prettyTime(date) {
        return date.toLocaleString(Qt.locale(), "h:mm");
    }

    Kirigami.Separator {
        Kirigami.FormData.isSection: true
    }

    QQC2.Label {
        wrapMode: Text.Wrap
        text: i18n("Night Color begins at %1", prettyTime(eveningTimings.begin))
    }
    QQC2.Label {
        wrapMode: Text.Wrap
        text: i18n("Color fully changed at %1", prettyTime(eveningTimings.end))
    }

    Item {
        Kirigami.FormData.isSection: true
    }

    QQC2.Label {
        wrapMode: Text.Wrap
        text: i18n("Night Color begins changing back at %1", prettyTime(morningTimings.begin))
    }
    QQC2.Label {
        wrapMode: Text.Wrap
        text: i18n("Normal coloration restored by %1", prettyTime(morningTimings.end))
    }
}
