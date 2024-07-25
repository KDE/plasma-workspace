/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.1

import org.kde.plasma.components 3.0 as PlasmaComponents3

RowLayout {
    id: toolButtonsLayout
    visible: menuItem.ListView.isCurrentItem

    // https://bugreports.qt.io/browse/QTBUG-108821
    readonly property bool hovered: actionToolButton.hovered || barcodeToolButton.hovered || editToolButton.hovered || deleteToolButton.hovered

    PlasmaComponents3.ToolButton {
        id: actionToolButton
        // TODO: only show for items supporting actions?
        icon.name: "system-run"

        display: PlasmaComponents3.AbstractButton.IconOnly
        text: i18nd("klipper", "Invoke action")

        onClicked: menuItem.triggerAction()

        PlasmaComponents3.ToolTip {
            text: actionToolButton.text
        }
        KeyNavigation.right: barcodeToolButton
    }
    PlasmaComponents3.ToolButton {
        id: barcodeToolButton
        icon.name: "view-barcode-qr"
        display: PlasmaComponents3.AbstractButton.IconOnly
        text: i18nd("klipper", "Show QR code")

        onClicked: menuItem.barcode()

        PlasmaComponents3.ToolTip {
            text: barcodeToolButton.text
        }
        KeyNavigation.right: editToolButton
    }
    PlasmaComponents3.ToolButton {
        id: editToolButton
        icon.name: "document-edit"
        enabled: !clipboardMenu.editing
        visible: menuItem.type === 0

        display: PlasmaComponents3.AbstractButton.IconOnly
        text: i18nd("klipper", "Edit contents")

        onClicked: menuItem.edit()

        PlasmaComponents3.ToolTip {
            text: editToolButton.text
        }
        KeyNavigation.right: deleteToolButton
    }
    PlasmaComponents3.ToolButton {
        id: deleteToolButton
        icon.name: "edit-delete"

        display: PlasmaComponents3.AbstractButton.IconOnly
        text: i18nd("klipper", "Remove from history")

        onClicked: menuItem.remove()

        PlasmaComponents3.ToolTip {
            text: deleteToolButton.text
        }
    }
}
