/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright (C) 2014 Kai Uwe Broulik <kde@privat.broulik.de>

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
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

ColumnLayout {
    RowLayout {
        Layout.fillWidth: true
        Item {
            width: units.gridUnit / 2 - parent.spacing
            height: 1
        }
        PlasmaComponents.TextField {
            id: filter
            placeholderText: i18n("Search")
            clearButtonShown: true
            Layout.fillWidth: true
        }
        PlasmaComponents.ToolButton {
            iconSource: "edit-delete"
            tooltip: i18n("Clear history")
            onClicked: clipboardSource.service("", "clearHistory")
        }
    }
    Menu {
        id: clipboardMenu
        model: PlasmaCore.SortFilterModel {
            sourceModel: clipboardSource.models.clipboard
            filterRole: "DisplayRole"
            filterRegExp: filter.text
        }
        supportsBarcodes: clipboardSource.data["clipboard"]["supportsBarcodes"]
        Layout.fillWidth: true
        Layout.fillHeight: true
        onItemSelected: clipboardSource.service(uuid, "select")
        onRemove: clipboardSource.service(uuid, "remove")
        onEdit: clipboardSource.edit(uuid)
        onBarcode: {
            var page = stack.push(barcodePage);
            page.show(uuid);
        }
        onAction: {
            clipboardSource.service(uuid, "action")
            clipboardMenu.view.currentIndex = 0
        }
    }
}
