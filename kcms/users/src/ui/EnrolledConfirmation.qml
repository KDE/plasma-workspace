/*
    SPDX-FileCopyrightText: 2024 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami

Item {
    ColumnLayout {

        anchors.centerIn: parent

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.round(Kirigami.Units.iconSizes.huge * 1.5)
            Layout.preferredHeight: Math.round(Kirigami.Units.iconSizes.huge * 1.5)

            source: "checkmark-symbolic"
            color: Kirigami.Theme.positiveTextColor
        }

        Kirigami.Heading {
            text: i18n("Finger Enrolled")

            Layout.fillWidth: true
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter

            wrapMode: Text.WordWrap
        }
    }
}
