/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.clock as PlasmaClock
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root

    readonly property bool softwareRendering: GraphicsInfo.api === GraphicsInfo.Software

    PlasmaComponents3.Label {
        text: Qt.formatTime(timeSource.dateTime, Qt.locale(), Locale.ShortFormat)
        textFormat: Text.PlainText
        style: root.softwareRendering ? Text.Outline : Text.Normal
        styleColor: root.softwareRendering ? Kirigami.Theme.backgroundColor : "transparent" //no outline, doesn't matter
        font.pointSize: Math.round(Kirigami.Theme.defaultFont.pointSize * 7.2)
        font.weight: Font.DemiBold
        font.letterSpacing: -3.0
        font.wordSpacing: 3.0
        renderType: Text.CurveRendering // looks better than QtTextRendering at large sizes
        Layout.alignment: Qt.AlignHCenter
    }
    PlasmaComponents3.Label {
        text: Qt.formatDate(timeSource.dateTime, Qt.locale(), Locale.LongFormat)
        textFormat: Text.PlainText
        style: root.softwareRendering ? Text.Outline : Text.Normal
        styleColor: root.softwareRendering ? Kirigami.Theme.backgroundColor : "transparent" //no outline, doesn't matter
        font.pointSize: Math.round(Kirigami.Theme.defaultFont.pointSize * 2.4)
        renderType: Text.CurveRendering // looks better than QtTextRendering at large sizes
        Layout.alignment: Qt.AlignHCenter
    }

    PlasmaClock.Clock {
        id: timeSource
        trackSeconds: Qt.locale().timeFormat(Locale.ShortFormat).includes("s")
    }
}
