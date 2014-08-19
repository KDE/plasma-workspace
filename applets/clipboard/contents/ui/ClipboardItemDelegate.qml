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
                visible: TypeRole == 2

                height: units.gridUnit * 4
                width: parent.width

                Component.onCompleted: {
                    if (TypeRole  == 2) {
                        print("DisplayRole: " + DisplayRole);
                        print(" sliced: " + DisplayRole.slice(7, DisplayRole.length));
                        var urls = DisplayRole.slice(7, DisplayRole.length).split("file://");
                        print("Model data inside delegate: " + urls);
                        for (var k in urls) {
                            print("_____________ KEY: " + k + " " + urls[k]);
                        }
                        previewList.model = urls
                    }
                }
                ListView {
                    id: previewList
//                     model: DisplayRole.split(" ")

                    property int itemWidth: units.gridUnit * 3
                    spacing: units.smallSpacing
//                     verticalSpacing: units.smallSpacing
                    orientation: Qt.Horizontal
                    anchors.fill: parent
                    //columns: parent.width / itemWidth
                    property int cellWidth: itemWidth
                    property int cellHeight: itemHeight * .66


                    delegate: KQuickControlsAddons.QPixmapItem {
                        id: previewPixmap
//                         width: parent.width
//                         height: width * (nativeHeight/nativeWidth)
                        //pixmap: DecorationRole
                        width: Math.round(height * 1.5)
                        height: Math.round(parent.height * 0.8)
                        y: Math.round(parent.height / 10)
                        //visible: TypeRole == 1 || TypeRole == 2
                        fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectCrop
                        Component.onCompleted: {
                            var service = clipboardSource.serviceForSource(UuidRole)
                            var operation = "preview";

                            function result(job) {
                                //print("result!..");
                                if (!job.error) {
                                    print("Cool!");
                                    print(" res: " + job.result["url"]);
                                    pixmap = job.result["preview"];
                                } else {
                                    print("Job failed");
                                }
                                //spixmap = job.result;

                                //print("ServiceJob error: " + job.error + " result: " + job.result + " op: " + job.operationName);
                            }

                            var operation = service.operationDescription(operation);
                            operation.urls = modelData;
                            //operation.password = password;
                            var serviceJob = service.startOperationCall(operation);
                            serviceJob.finished.connect(result);
                            print("JOb started: " + modelData);

                        }
                        Rectangle {
                            border.width: 1
                            border.color: "black"
                            color: "transparent"
                            anchors.fill: parent
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
