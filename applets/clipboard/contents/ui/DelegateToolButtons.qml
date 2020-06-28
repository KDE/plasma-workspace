/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright     2014 Sebastian Kügler <sebas@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

import QtQuick 2.0

import org.kde.plasma.components 3.0 as PlasmaComponents3

Row {
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
