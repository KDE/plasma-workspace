/*
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.plasmoid 2.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami

import org.kde.prison 1.0 as Prison

ColumnLayout {
    id: barcodeView

    property alias text: barcodeItem.content

    Keys.onPressed: event => {
        if (event.key == Qt.Key_Escape) {
            stack.pop()
            event.accepted = true;
        }
    }

    property var header: PlasmaExtras.PlasmoidHeading {
        RowLayout {
            anchors.fill: parent
            PlasmaComponents3.Button {
                Layout.fillWidth: true
                icon.name: "go-previous-view"
                text: i18n("Return to Clipboard")
                onClicked: stack.pop()
            }

            Component {
                id: menuItemComponent
                PlasmaComponents3.MenuItem { }
            }

            PlasmaComponents3.Menu {
                id: menu

                onClosed: {
                    configureButton.checked = false;
                }

                Component.onCompleted: {
                    [
                        {text: i18n("QR Code"), type: Prison.Barcode.QRCode},
                        {text: i18n("Data Matrix"), type: Prison.Barcode.DataMatrix},
                        {text: i18nc("Aztec barcode", "Aztec"), type: Prison.Barcode.Aztec},
                        {text: i18n("Code 39"), type: Prison.Barcode.Code39},
                        {text: i18n("Code 93"), type: Prison.Barcode.Code93},
                        {text: i18n("Code 128"), type: Prison.Barcode.Code128}
                    ].forEach((item) => {
                        let menuItem = menuItemComponent.createObject(menu, {
                            text: item.text,
                            checkable: true,
                            autoExclusive: true,
                            checked: Qt.binding(() => {
                                return barcodeItem.barcodeType === item.type;
                            })
                        });
                        menuItem.clicked.connect(() => {
                            barcodeItem.barcodeType = item.type;
                            Plasmoid.configuration.barcodeType = item.type;
                        });
                        menu.addItem(menuItem);
                    });
                }
            }
            PlasmaComponents3.ToolButton {
                id: configureButton
                checkable: true
                icon.name: "configure"

                display: PlasmaComponents3.AbstractButton.IconOnly
                text: i18nc("@action:button", "Change the barcode type")

                onClicked: menu.popup()

                PlasmaComponents3.ToolTip {
                    text: parent.text
                }
            }
        }
    }

    Item {
        Layout.fillWidth: parent
        Layout.fillHeight: parent
        Layout.topMargin: Kirigami.Units.smallSpacing

        Prison.Barcode {
            id: barcodeItem
            readonly property bool valid: implicitWidth > 0 && implicitHeight > 0 && implicitWidth <= width && implicitHeight <= height
            anchors.fill: parent
            barcodeType: Plasmoid.configuration.barcodeType
            // Cannot set visible to false as we need it to re-render when changing its size
            opacity: valid ? 1 : 0

            Drag.dragType: Drag.Automatic
            Drag.supportedActions: Qt.CopyAction

            HoverHandler {
                enabled: barcodeItem.valid
                cursorShape: Qt.OpenHandCursor
            }

            DragHandler {
                id: dragHandler
                enabled: barcodeItem.valid

                onActiveChanged: {
                    if (active) {
                        barcodeItem.grabToImage((result) => {
                            barcodeItem.Drag.mimeData = {
                                "image/png": result.image,
                            };
                            barcodeItem.Drag.active = dragHandler.active;
                        });
                    } else {
                        barcodeItem.Drag.active = false;
                    }
                }
            }
        }

        PlasmaComponents3.Label {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: i18nc("@info:status", "Creating barcode failed")
            wrapMode: Text.WordWrap
            visible: barcodeItem.implicitWidth === 0 && barcodeItem.implicitHeight === 0
        }

        PlasmaComponents3.Label {
            anchors.fill: parent
            leftPadding: Kirigami.Units.gridUnit * 2
            rightPadding: Kirigami.Units.gridUnit * 2
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: i18nc("@info:usagetip", "There is not enough space to display the barcode. Try resizing this applet.")
            wrapMode: Text.WordWrap
            visible: barcodeItem.implicitWidth > barcodeItem.width || barcodeItem.implicitHeight > barcodeItem.height
        }
    }
}
