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
    
    property alias showToolButton: toolButton.visible
    property alias toolButtonName: toolButton.text
    property alias toolButtonIcon: toolButton.icon.name

    property int releaseCookie: 0

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

    PlasmaComponents3.ToolButton {
        id: toolButton
        visible: false

        display: PlasmaComponents3.AbstractButton.IconOnly
        PlasmaComponents3.ToolTip {
            text: parent.text
        }
        text: i18nc("@action:button Stop an app from blocking automatic sleep and screen locking after inactivity", "Unblock")
        icon.name: "edit-delete-remove"

        Keys.onPressed: (event) => {
            if (event.key == Qt.Key_Space || event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                clicked();
            }
        }

        onClicked: {
            inhibitionReleaseRequested(releaseCookie);
        }
    }
}
