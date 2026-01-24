/*
 * SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kirigami 2.19 as Kirigami
import org.kde.plasma.components 3.0 as PC3

ColumnLayout {
    id: root

    property string title
    property string message

    anchors.verticalCenter: parent.verticalCenter
    anchors.left: parent
    anchors.leftMargin: Kirigami.Units.largeSpacing
    anchors.right: parent
    anchors.rightMargin: Kirigami.Units.largeSpacing
    spacing: Kirigami.Units.largeSpacing

    QQC2.Label {
        text: title
        Layout.alignment: Qt.AlignHCenter
        horizontalAlignment: Text.AlignHCenter
    }

    QQC2.TextArea {
        visible: message !== ""
        text: message
        readOnly: true
        wrapMode: TextEdit.Wrap
        Layout.fillWidth: true
    }
}
