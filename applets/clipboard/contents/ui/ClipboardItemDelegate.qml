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
    property int maximumNumberOfPreviews: 4
    signal itemSelected(string uuid)
    signal remove(string uuid)
    signal edit(string uuid)
    signal barcode(string uuid)
    signal action(string uuid)

    width: parent.width
    height: Math.max(label.height, toolButtonsLayout.implicitHeight) + 2 * units.smallSpacing

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onClicked: menuItem.itemSelected(UuidRole)

        onEntered: menuListView.currentIndex = index
        onExited: menuListView.currentIndex = -1

        Item {
            id: label
            height: childrenRect.height
            anchors {
                left: parent.left
                leftMargin: units.smallSpacing
                right: parent.right
                rightMargin: units.smallSpacing
                verticalCenter: parent.verticalCenter
            }
            PlasmaComponents.Label {
                height: implicitHeight
                width: parent.width
                maximumLineCount: 3
                text: DisplayRole
                visible: TypeRole == 0 // TypeRole: 0: Text, 1: Image, 2: Url
                textFormat: Text.PlainText
            }
            KQuickControlsAddons.QPixmapItem {
                id: previewPixmap
                width: parent.width
                height: width * (nativeHeight/nativeWidth)
                pixmap: DecorationRole
                visible: TypeRole == 1
                fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectFit
            }
            Item {
                id: previewItem
                visible: TypeRole == 2

                height: visible ? units.gridUnit * 4 : 0
                width: parent.width

                ListView {
                    id: previewList
                    model: TypeRole == 2 ? DisplayRole.split(" ", maximumNumberOfPreviews) : 0
                    property int itemWidth: units.gridUnit * 4
                    property int itemHeight: Math.round(itemWidth * 0.66)
                    interactive: contentWidth > width

                    spacing: units.smallSpacing
                    orientation: Qt.Horizontal
                    anchors.fill: parent

                    delegate: KQuickControlsAddons.QPixmapItem {
                        id: previewPixmap

                        width: previewList.itemWidth
                        height:  previewList.itemHeight
                        y: Math.round((parent.height - previewList.itemHeight) / 2)

                        fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectCrop
                        Component.onCompleted: {

                            function result(job) {
                                if (!job.error) {
                                    pixmap = job.result["preview"];
                                }
                            }

                            var service = clipboardSource.serviceForSource(UuidRole)
                            var operation = service.operationDescription("preview");
                            operation.url = modelData;
                            operation.previewWidth = previewPixmap.width;
                            operation.previewHeight = previewPixmap.height;
                            var serviceJob = service.startOperationCall(operation);
                            serviceJob.finished.connect(result);
                        }
//                         Rectangle {
//                             border.width: 1
//                             border.color: "black"
//                             color: "transparent"
//                             anchors.fill: parent
//                         }
                    }
                }

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
                enabled: !clipboardSource.editing
                visible: TypeRole != 2
                tooltip: i18n("Edit contents")
                onClicked: menuItem.edit(UuidRole)
            }
            PlasmaComponents.ToolButton {
                iconSource: "edit-delete"
                tooltip: i18n("Remove from history")
                onClicked: menuItem.remove(UuidRole)
            }

            Component.onCompleted: {
                toolButtonsLayout.visible = Qt.binding(function () { return menuListView.currentIndex == index; });
            }
        }
    }
}
