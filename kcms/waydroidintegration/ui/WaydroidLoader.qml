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
    property string text

    anchors.centerIn: parent
    spacing: Kirigami.Units.largeSpacing

    PC3.BusyIndicator {
        Layout.alignment: Qt.AlignHCenter
        implicitHeight: Kirigami.Units.iconSizes.huge
        implicitWidth: Kirigami.Units.iconSizes.huge

        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    }

    QQC2.Label {
        text: root.text
        horizontalAlignment: Text.AlignHCenter
        Layout.alignment: Qt.AlignHCenter
    }
}