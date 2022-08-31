/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.5 as QQC2

ColumnLayout {

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

    QQC2.Label {
        wrapMode: Text.Wrap
        Layout.fillWidth: true
        horizontalAlignment: Qt.AlignHCenter
        text: i18n("Color temperature begins changing to night time at %1 and is fully changed by %2", prettyTime(eveningTimings.begin), prettyTime(eveningTimings.end))
    }

    QQC2.Label {
        wrapMode: Text.Wrap
        Layout.fillWidth: true
        horizontalAlignment: Qt.AlignHCenter
        text: i18n("Color temperature begins changing to day time at %1 and is fully changed by %2", prettyTime(morningTimings.begin), prettyTime(morningTimings.end))
    }
}
