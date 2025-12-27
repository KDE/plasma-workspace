/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import Qt5Compat.GraphicalEffects

import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

Rectangle {
    id: root

    property alias text: label.text
    property Item icon

    // Add to the horizontal sides of the label to add a bit of padding and stay
    // within `icon.width` if `icon` is set.
    //
    // We don't use padding or margin properties because that puts a hard limit
    // on where the text can be placed by Label. The first priority should be
    // fitting small text within the bounds so that it is readable.
    //
    // `implicitWidth` depends on height when `icon` is set.
    implicitWidth: Math.round(label.implicitWidth + Math.min(fontMetrics.descent, radius) * 2)
    width: icon ? Math.min(icon.width, Math.max(height, implicitWidth)) : Math.max(height, implicitWidth)
    // Keep the badge in the bounds of icon and try to keep text readable if
    // `icon.height / 4` is too small.
    // `iconSizes.small` is the size for system tray icons with a 20px panel.
    // We need to add 4 to give space for the border and a bit of space between the border and the text.
    implicitHeight: Math.round(Kirigami.Units.iconSizes.small / 1.75) + 4
    height: icon ? Math.min(icon.height, Math.max(Math.round(icon.height / 4) + 4, implicitHeight)) : implicitHeight

    color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.9)
    radius: Math.min(Kirigami.Units.cornerRadius, height / 2)
    border.color: Qt.alpha(Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
    border.width: 1

    FontMetrics {
        id: fontMetrics
        font: label.font
    }

    PlasmaComponents3.Label {
        id: label
        anchors.fill: parent
        font.pixelSize: Math.max(root.implicitHeight, root.height) - 4
        textFormat: Text.PlainText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
