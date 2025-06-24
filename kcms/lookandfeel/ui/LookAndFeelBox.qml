/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami

QtControls.AbstractButton {
    id: button

    property alias preview: previewImage.source

    contentItem: Column {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.ShadowedRectangle {
            id: delegate
            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            implicitWidth: implicitHeight * 1.6
            implicitHeight: Kirigami.Units.gridUnit * 5
            color: {
                if (button.pressed) {
                    return Kirigami.Theme.highlightColor;
                } else if (button.hovered || button.visualFocus) {
                    return Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.5);
                } else {
                    return Kirigami.Theme.backgroundColor;
                }
            }
            radius: Kirigami.Units.cornerRadius
            shadow.xOffset: 0
            shadow.yOffset: 2
            shadow.size: 10
            shadow.color: Qt.rgba(0, 0, 0, 0.3)

            Image {
                id: previewImage
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                asynchronous: true
                sourceSize: Qt.size(width * Screen.devicePixelRatio, height * Screen.devicePixelRatio)
            }
        }

        QtControls.Label {
            id: label
            width: delegate.implicitWidth
            text: button.text
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
