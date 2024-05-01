/*
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

RowLayout {
    property alias iconSource: iconItem.source
    property alias text: label.text
    property alias actions: actionToolBar.actions
    
    spacing: Kirigami.Units.smallSpacing

    Kirigami.Icon {
        id: iconItem
        Layout.preferredWidth: Kirigami.Units.iconSizes.small
        Layout.preferredHeight: Kirigami.Units.iconSizes.small
        visible: valid
    }

    PlasmaComponents3.Label {
        id: label
        Layout.fillWidth: true
        font: Kirigami.Theme.smallFont
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 4
    }

    Kirigami.ActionToolBar {
        id: actionToolBar
        Layout.minimumWidth: Kirigami.Units.iconSizes.large
        Layout.alignment: Qt.AlignRight
        visible: actions.length > 0
    }
}
