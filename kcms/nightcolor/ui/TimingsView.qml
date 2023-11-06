/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

ColumnLayout {
    id: root

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
        text: i18n("Color temperature begins changing to night time at %1 and is fully changed by %2",
            root.prettyTime(root.eveningTimings.begin), root.prettyTime(root.eveningTimings.end))
    }

    QQC2.Label {
        wrapMode: Text.Wrap
        Layout.fillWidth: true
        horizontalAlignment: Qt.AlignHCenter
        text: i18n("Color temperature begins changing to day time at %1 and is fully changed by %2",
            root.prettyTime(root.morningTimings.begin), root.prettyTime(root.morningTimings.end))
    }
}
