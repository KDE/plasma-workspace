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
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

PlasmaComponents.ListItem {
    id: menuItem

    property alias supportsBarcodes: barcodeToolButton.visible
    property int maximumNumberOfPreviews: Math.floor(width / (units.gridUnit * 4 + units.smallSpacing))
    signal itemSelected(string uuid)
    signal remove(string uuid)
    signal edit(string uuid)
    signal barcode(string uuid)
    signal action(string uuid)

    width: parent.width - units.gridUnit * 2
    height: Math.max(label.height, toolButtonsLayout.implicitHeight) + 2 * units.smallSpacing

   x: -listMargins.left

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onClicked: menuItem.itemSelected(UuidRole)

        onEntered: menuListView.currentIndex = index
        onExited: menuListView.currentIndex = -1

        //Rectangle { visible: main.debug; color: "transparent"; opacity: 1; anchors.fill: parent; border.width: 1 }
        Item {
            id: label
            height: childrenRect.height
            anchors {
                left: parent.left
                leftMargin: units.gridUnit / 2
                right: parent.right
                //rightMargin: units.smallSpacing
                verticalCenter: parent.verticalCenter
            }
            PlasmaComponents.Label {
                height: implicitHeight
                //width: parent.width - units.gridUnit * 4
                anchors {
                    left: parent.left
                    //leftMargin: units.smallSpacing
                    right: parent.right
                    rightMargin: units.gridUnit * 2
                    //rightMargin: units.smallSpacing
                    //verticalCenter: parent.verticalCenter
                }
                maximumLineCount: 3
                text: DisplayRole.trim()
                visible: TypeRole == 0 // TypeRole: 0: Text, 1: Image, 2: Url
                //textFormat: Text.PlainText
                elide: Text.ElideRight
                wrapMode: Text.Wrap
            }
            KQuickControlsAddons.QPixmapItem {
                id: previewPixmap
                width: parent.width
                height: Math.round(width * (nativeHeight/nativeWidth) + units.smallSpacing * 2)
                pixmap: DecorationRole
                visible: TypeRole == 1
                fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectFit
            }
            Item {
                id: previewItem
                visible: TypeRole == 2

                height: visible ? (units.gridUnit * 4 + units.smallSpacing * 2) : 0
                width: parent.width

                ListView {
                    id: previewList
                    model: TypeRole == 2 ? DisplayRole.split(" ", maximumNumberOfPreviews) : 0
                    property int itemWidth: units.gridUnit * 4
                    property int itemHeight: units.gridUnit * 4
                    interactive: contentWidth > width

                    spacing: units.smallSpacing
                    orientation: Qt.Horizontal
                    anchors.fill: parent

                    delegate: Item {
                        width: previewList.itemWidth
                        height:  previewList.itemHeight
                        y: Math.round((parent.height - previewList.itemHeight) / 2)
                        clip: true

                        KQuickControlsAddons.QPixmapItem {
                            id: previewPixmap

                            anchors.centerIn: parent
                            //fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectFit

                            Component.onCompleted: {

                                function result(job) {
                                    print("---> result" + job.result.url + job.result.iconName);
                                    if (!job.error) {
                                        pixmap = job.result.preview;
                                        previewPixmap.width = job.result.previewWidth
                                        previewPixmap.height = job.result.previewHeight
                                        print("set size: " +previewPixmap.width  + "x" + previewPixmap.height)
                                    } else {
                                        print("parentsizing");
                                        previewPixmap.width = parent.width
                                        previewPixmap.height = parent.height
                                    }
                                }

                                var service = clipboardSource.serviceForSource(UuidRole)
                                var operation = service.operationDescription("preview");
                                operation.url = modelData;
//                                 operation.previewWidth = previewPixmap.width;
//                                 operation.previewHeight = previewPixmap.height;
                                operation.previewWidth = previewList.itemWidth * 2;
                                operation.previewHeight = previewList.itemHeight * 2;
                                print("Requesting size: " + operation.previewWidth + "x" + operation.previewHeight)
                                var serviceJob = service.startOperationCall(operation);
                                serviceJob.finished.connect(result);
                            }
                        }
                        Rectangle {
                            id: overlay
                            color: theme.textColor
                            opacity: 0.6
                            height: units.gridUnit
                            anchors {
                                left: parent.left
                                right: parent.right
                                bottom: parent.bottom
                            }
                        }
                        PlasmaComponents.Label {
                            font.pointSize: theme.smallestFont.pointSize
                            color: theme.backgroundColor
                            maximumLineCount: 1
                            anchors {
                                verticalCenter: overlay.verticalCenter
                                left: overlay.left
                                right: overlay.right
                                leftMargin: units.smallSpacing
                                rightMargin: units.smallSpacing
                            }
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var u = modelData.split("/");
                                return u[u.length - 1];

                            }
                        }
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
