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

import org.kde.plasma.components 2.0 as PlasmaComponents

Row {
    id: toolButtonsLayout
    visible: menuItem.ListView.isCurrentItem

    PlasmaComponents.ToolButton {
        // TODO: only show for items supporting actions?
        iconSource: "system-run"
        tooltip: i18n("Invoke action")
        onClicked: menuItem.action(UuidRole)
    }
    PlasmaComponents.ToolButton {
        id: barcodeToolButton
        iconSource: "view-barcode"
        tooltip: i18n("Show barcode")
        onClicked: menuItem.barcode(UuidRole)
    }
    PlasmaComponents.ToolButton {
        iconSource: "document-edit"
        enabled: !clipboardSource.editing
        visible: TypeRole === 0
        tooltip: i18n("Edit contents")
        onClicked: menuItem.edit(UuidRole)
    }
    PlasmaComponents.ToolButton {
        iconSource: "edit-delete"
        tooltip: i18n("Remove from history")
        onClicked: menuItem.remove(UuidRole)
    }
}
