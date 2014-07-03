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
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: main
    anchors.fill: parent
    Plasmoid.switchWidth: units.gridUnit * 10
    Plasmoid.switchHeight: units.gridUnit * 10
    Plasmoid.status: (clipboardSource.models.clipboard.count > 0) ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
    Plasmoid.toolTipMainText: i18n("Clipboard Contents")
    Plasmoid.toolTipSubText: clipboardSource.data["clipboard"]["current"]
    Plasmoid.icon: "klipper"

    PlasmaCore.DataSource {
        id: clipboardSource
        engine: "org.kde.plasma.clipboard"
        connectedSources: "clipboard"
        function service(uuid, op) {
            var service = clipboardSource.serviceForSource(uuid);
            var operation = service.operationDescription(op);
            service.startOperationCall(operation);
        }
    }

    Plasmoid.fullRepresentation: Item {
        id: dialogItem
        Layout.minimumWidth: units.iconSizes.medium * 9
        Layout.minimumHeight: units.gridUnit * 13

        PlasmaComponents.Highlight {
            id: highlightItem

            property Item trackingItem
            onTrackingItemChanged: {
                if (trackingItem) {
                    y = trackingItem.mapToItem(main, 0, 0).y
                }
            }

            hover: true
            width: clipboardMenu.width
            x: 0
            y: 0
            height: trackingItem.height
            visible: true

            Behavior on opacity {
                NumberAnimation {
                    duration: units.longDuration
                    easing: Easing.InOutQuad
                }
            }
            Behavior on y {
                NumberAnimation {
                    duration: units.longDuration
                    easing: Easing.InOutQuad
                }
            }
        }
        Connections {
            target: clipboardMenu.flickableItem
            onContentYChanged: {
                highlightItem.trackingItem = null
            }
        }

        ColumnLayout {
            anchors.fill: parent
            RowLayout {
                Layout.fillWidth: true
                PlasmaComponents.TextField {
                    id: filter
                    placeholderText: i18n("Search")
                    clearButtonShown: true
                    Layout.fillWidth: true
                }
                PlasmaComponents.ToolButton {
                    iconSource: "edit-clear-history"
                    tooltip: i18n("Clear history")
                    onClicked: clipboardSource.service("", "clearHistory")
                }
                PlasmaComponents.ToolButton {
                    iconSource: "configure"
                    tooltip: i18n("Configure")
                    onClicked: clipboardSource.service("", "configureKlipper")
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
                onEdit: clipboardSource.service(uuid, "edit")
                onBarcode: clipboardSource.service(uuid, "barcode")
                onAction: clipboardSource.service(uuid, "action")
            }
        }
    }
}
