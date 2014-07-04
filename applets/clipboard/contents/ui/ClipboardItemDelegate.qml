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
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

PlasmaComponents.ListItem {
    id: menuItem

    property alias supportsBarcodes: barcodeToolButton.visible
    signal itemSelected(string uuid)
    signal remove(string uuid)
    signal edit(string uuid)
    signal barcode(string uuid)
    signal action(string uuid)

    width: parent.width
    height: Math.max(label.height, toolButtonsLayout.implicitHeight) + highlightItem.marginHints.top + highlightItem.marginHints.bottom

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onClicked: menuItem.itemSelected(UuidRole)

        onEntered: {
            highlightItem.trackingItem = menuItem
            highlightItem.width = menuItem.width
            highlightItem.height = menuItem.height
        }
    }

    Item {
        id: label
        height: childrenRect.height
        anchors {
            left: parent.left
            leftMargin: highlightItem.marginHints.left
            right: parent.right
            rightMargin: highlightItem.marginHints.right
            verticalCenter: parent.verticalCenter
        }
        PlasmaComponents.Label {
            height: implicitHeight
            width: parent.width
            text: DisplayRole
            visible: TypeRole != 1 // TypeRole: 0: Text, 1: Image, 2: Url
            textFormat: Text.PlainText
        }
        KQuickControlsAddons.QPixmapItem {
            width: parent.width
            height: width * (nativeHeight/nativeWidth)
            pixmap: DecorationRole
            visible: TypeRole == 1
            fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectFit
        }
    }

    RowLayout {
        id: toolButtonsLayout
        anchors {
            right: label.right
            verticalCenter: parent.verticalCenter
        }
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
            tooltip: i18n("Edit contents")
            onClicked: menuItem.edit(UuidRole)
        }
        PlasmaComponents.ToolButton {
            iconSource: "edit-delete"
            tooltip: i18n("Remove from history")
            onClicked: menuItem.remove(UuidRole)
        }
        Component.onCompleted: {
            toolButtonsLayout.visible = Qt.binding(function () { return highlightItem.trackingItem == menuItem; });
        }
    }
}
