/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.plasma5support as P5Support
import org.kde.kirigami as Kirigami

Item {
    id: root

    readonly property bool softwareRendering: GraphicsInfo.api === GraphicsInfo.Software
    property bool alwaysShowClock: true
    property real uiOpacity: 1

    implicitWidth: contentItem.implicitWidth
    implicitHeight: contentItem.implicitHeight

    property Item contentItem: ColumnLayout {
        parent: root
        anchors.centerIn: parent
        width: Math.min(implicitWidth, parent.width)
        height: Math.min(implicitHeight, parent.height)
        spacing: 0
        opacity: root.alwaysShowClock ? 1 : root.uiOpacity
        PlasmaComponents3.Label {
            id: timeLabel
            text: Qt.formatTime(timeSource.data["Local"]["DateTime"], Qt.locale(), Locale.ShortFormat)
            textFormat: Text.PlainText
            style: root.softwareRendering ? Text.Outline : Text.Normal
            styleColor: Kirigami.Theme.backgroundColor //no outline, doesn't matter
            font.pointSize: Math.max(Kirigami.Theme.defaultFont.pointSize * 4.8)
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        PlasmaComponents3.Label {
            text: Qt.formatDate(timeSource.data["Local"]["DateTime"], Qt.locale(), Locale.LongFormat)
            textFormat: Text.PlainText
            style: root.softwareRendering ? Text.Outline : Text.Normal
            styleColor: Kirigami.Theme.backgroundColor //no outline, doesn't matter
            font.pointSize: timeLabel.font.pointSize / 2
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    property Item shadow: MultiEffect {
        z: -1
        parent: root
        source: root.contentItem
        width: source.width
        height: source.height
        x: source.x
        y: source.y
        visible: !root.softwareRendering && opacity > 0 && source.opacity > 0
        opacity: 1 - root.uiOpacity
        shadowEnabled: true
        shadowColor: "black"
        blurMax: 16
    }

    P5Support.DataSource {
        id: timeSource
        engine: "time"
        connectedSources: ["Local"]
        interval: 1000
    }
}
