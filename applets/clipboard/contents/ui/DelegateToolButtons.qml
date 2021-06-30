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

    PlasmaComponents3.ToolButton {
        // TODO: only show for items supporting actions?
        icon.name: "system-run"
        onClicked: menuItem.action(UuidRole)

        PlasmaComponents3.ToolTip {
            text: i18n("Invoke action")
        }
    }
    PlasmaComponents3.ToolButton {
        id: barcodeToolButton
        icon.name: "view-barcode-qr"
        visible: supportsBarcodes
        onClicked: menuItem.barcode(DisplayRole)

        PlasmaComponents3.ToolTip {
            text: i18n("Show barcode")
        }
    }
    PlasmaComponents3.ToolButton {
        icon.name: "document-edit"
        enabled: !clipboardSource.editing
        visible: TypeRole === 0
        onClicked: menuItem.edit(UuidRole)

        PlasmaComponents3.ToolTip {
            text: i18n("Edit contents")
        }
    }
    PlasmaComponents3.ToolButton {
        icon.name: "edit-delete"
        onClicked: menuItem.remove(UuidRole)

        PlasmaComponents3.ToolTip {
            text: i18n("Remove from history")
        }
    }
}
