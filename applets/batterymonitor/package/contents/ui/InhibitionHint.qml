/*
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.kirigami 2.20 as Kirigami

RowLayout {
    property alias iconSource: iconItem.source
    property alias text: label.text

    spacing: Kirigami.Units.smallSpacing

    PlasmaCore.IconItem {
        id: iconItem
        Layout.preferredWidth: Kirigami.Units.iconSizes.small
        Layout.preferredHeight: Kirigami.Units.iconSizes.small
        visible: valid
    }

    PlasmaComponents3.Label {
        id: label
        Layout.fillWidth: true
        font: Kirigami.Theme.smallFont
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 4
    }
}
