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
    // `implicitWidth` depends on height when `icon` is set.
    implicitWidth: Math.round(label.implicitWidth + label.totalHorizontalPadding)
    width: icon ? Math.min(icon.width, Math.max(height, implicitWidth)) : Math.max(height, implicitWidth)
    // Keep the badge in the bounds of icon and try to keep text readable if
    // `icon.height / 4` is too small.
    implicitHeight: label.minimumPixelSize + label.totalVerticalPadding
    height: icon ? Math.min(icon.height, Math.max(Math.round(icon.height / 4) + label.totalVerticalPadding, implicitHeight)) : implicitHeight

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
        // We need to add padding to give space for the border and a bit of space
        // between the border and the text.
        // We don't use actual padding or margin properties because that puts a
        // hard limit on where the text can be placed by Label. The priority is
        // fitting small text within the bounds so that it is readable. This
        // also gives FreeType the best chance to make use of hinting and sub-
        // pixel rendering to keep the text readable.
        readonly property real totalVerticalPadding: root.border.width * 4
        // Padding is at least `totalVerticalPadding` and at most the larger of
        // the descent and the corner radius. We use descent and corner radius
        // to keep the text in the visual bounds and keep the horizontal padding
        // visually similar to the space around numbers reserved for diacritics.
        readonly property real totalHorizontalPadding: Math.max(label.totalVerticalPadding, Math.min(fontMetrics.descent, root.radius) * 2)
        // The size of system tray icons with a 20px panel is 16x16 (small).
        // For larger panel sizes, the minimum icon size is 22x22 (smallMedium).
        // 9 is a magic pixel size that is not too big/small for badges on 16x16
        // and 22x22 system tray icons with the Noto Sans font.
        //
        // There is no good way to find the perfect size for all fonts that is
        // readable and space efficient. fontSizeMode isn't really viable here
        // despite it being the perfect API for the job in theory. It leaves a
        // lot of empty space around the text to make room for things like
        // diacritics, which makes the text smaller. We mostly use badges for
        // numbers and percents, so the priority is keeping those readable.
        //
        // minimumPixelSize doesn't actually do anything special here, it's just
        // a conveniently named existing property. Normally, it only works when
        // fontSizeMode is used.
        minimumPixelSize: 9
        anchors.fill: parent
        font.pixelSize: Math.max(minimumPixelSize, root.height - totalVerticalPadding)
        textFormat: Text.PlainText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
