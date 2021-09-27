/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.8
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.5 as QQC2
import org.kde.plasma.core 2.0 as PlasmaCore

ColumnLayout {
    readonly property bool softwareRendering: GraphicsInfo.api === GraphicsInfo.Software

    QQC2.Label {
        text: Qt.formatTime(timeSource.data["Local"]["DateTime"])
        color: PlasmaCore.ColorScope.textColor
        style: softwareRendering ? Text.Outline : Text.Normal
        styleColor: softwareRendering ? PlasmaCore.ColorScope.backgroundColor : "transparent" //no outline, doesn't matter
        font.pointSize: 48
        Layout.alignment: Qt.AlignHCenter
    }
    QQC2.Label {
        text: Qt.formatDate(timeSource.data["Local"]["DateTime"], Qt.DefaultLocaleLongDate)
        color: PlasmaCore.ColorScope.textColor
        style: softwareRendering ? Text.Outline : Text.Normal
        styleColor: softwareRendering ? PlasmaCore.ColorScope.backgroundColor : "transparent" //no outline, doesn't matter
        font.pointSize: 24
        Layout.alignment: Qt.AlignHCenter
    }
    PlasmaCore.DataSource {
        id: timeSource
        engine: "time"
        connectedSources: ["Local"]
        interval: 1000
    }
}
