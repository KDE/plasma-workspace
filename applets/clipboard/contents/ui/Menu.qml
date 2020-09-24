/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

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
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents // For Highlight

import org.kde.kirigami 2.12 as Kirigami

PlasmaExtras.ScrollArea {
    id: menu
    property alias view: menuListView
    property alias model: menuListView.model
    property bool supportsBarcodes
    signal itemSelected(string uuid)
    signal remove(string uuid)
    signal edit(string uuid)
    signal barcode(string text)
    signal action(string uuid)

    ListView {
        id: menuListView
        focus: true

        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        highlight: PlasmaComponents.Highlight { }
        highlightMoveDuration: 0
        highlightResizeDuration: 0
        currentIndex: -1

        delegate: ClipboardItemDelegate {
            width: menuListView.width
            supportsBarcodes: menu.supportsBarcodes

            onItemSelected: menu.itemSelected(uuid)
            onRemove: menu.remove(uuid)
            onEdit: menu.edit(uuid)
            onBarcode: menu.barcode(text)
            onAction: menu.action(uuid)
        }

        PlasmaExtras.PlaceholderMessage {
            id: emptyHint

            anchors.centerIn: parent
            width: parent.width - (PlasmaCore.Units.largeSpacing * 4)

            visible: menuListView.count === 0
            text: i18n("Clipboard is empty")
        }
    }
}
