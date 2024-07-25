/*
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami

import org.kde.prison 1.0 as Prison

Item {
    id: barcodeView

    required property bool expanded
    required property T.StackView stack
    required property string barcodeType

    readonly property bool valid: barcodeItem.implicitWidth > 0 && barcodeItem.implicitHeight > 0
    readonly property bool fit: barcodeItem.implicitWidth <= barcodeItem.width && barcodeItem.implicitHeight <= barcodeItem.height
    property alias text: barcodeItem.content

    readonly property var barcodeMap: [
        {text: i18nd("klipper", "QR Code"), type: Prison.Barcode.QRCode, code: "QRCode"},
        {text: i18nd("klipper", "Data Matrix"), type: Prison.Barcode.DataMatrix, code: "DataMatrix"},
        {text: i18ndc("klipper", "Aztec barcode", "Aztec"), type: Prison.Barcode.Aztec, code: "Aztec"},
        {text: i18nd("klipper", "Code 39"), type: Prison.Barcode.Code39, code: "Code39"},
        {text: i18nd("klipper", "Code 93"), type: Prison.Barcode.Code93, code: "Code93"},
        {text: i18nd("klipper", "Code 128"), type: Prison.Barcode.Code128, code: "Code128"}
    ]

    Keys.onPressed: event => {
        if (event.key == Qt.Key_Escape) {
            barcodeView.stack.popCurrentItem();
            event.accepted = true;
        }
    }

    property PlasmaExtras.PlasmoidHeading header: PlasmaExtras.PlasmoidHeading {
        RowLayout {
            anchors.fill: parent
            PlasmaComponents3.Button {
                Layout.fillWidth: true
                icon.name: "go-previous-view"
                text: i18n("Return to Clipboard")
                onClicked: barcodeView.stack.popCurrentItem()
            }

            PlasmaComponents3.Menu {
                id: menu
            }

            Instantiator {
                id: menuItemInstantiator
                active: barcodeView.expanded && menu.opened
                asynchronous: true
                delegate: PlasmaComponents3.MenuItem {
                    required property var modelData
                    text: modelData.text
                    checkable: true
                    autoExclusive: true
                    checked: barcodeItem.barcodeType === modelData.type

                    onClicked: {
                        barcodeView.barcodeType = modelData.code;
                        Plasmoid.configuration.barcodeType = modelData.code;
                    }
                }
                model: barcodeView.barcodeMap

                onObjectAdded: (index, object) => menu.insertItem(index, object as PlasmaComponents3.MenuItem)
            }

            PlasmaComponents3.ToolButton {
                id: configureButton
                checkable: true
                checked: menu.opened
                icon.name: "configure"

                display: PlasmaComponents3.AbstractButton.IconOnly
                text: i18n("Change the QR code type")

                onClicked: menu.opened ? menu.close() : menu.popup()

                PlasmaComponents3.ToolTip {
                    text: configureButton.text
                }
            }
        }
    }

    Prison.Barcode {
        id: barcodeItem
        anchors.fill: parent
        anchors.topMargin: Kirigami.Units.smallSpacing
        barcodeType: barcodeView.barcodeMap.find(data => data.code === barcodeView.barcodeType)?.type ?? barcodeView.barcodeMap[0].type
        // Cannot set visible to false as we need it to re-render when changing its size
        opacity: barcodeView.valid && barcodeView.fit ? 1 : 0

        Accessible.name: barcodeView.barcodeMap.find(data => data.type === barcodeItem.barcodeType)?.text ?? barcodeView.barcodeMap[0].text
        Accessible.role: Accessible.Graphic
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction

        HoverHandler {
            enabled: barcodeView.valid && barcodeView.fit
            cursorShape: Qt.OpenHandCursor
        }

        DragHandler {
            id: dragHandler
            enabled: barcodeView.valid && barcodeView.fit

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

    PlasmaExtras.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.gridUnit * 4)
        visible: !barcodeView.valid || !barcodeView.fit
        iconName: barcodeView.valid ? "dialog-transform" : "data-error"
        text: barcodeView.valid ? i18nd("klipper", "There is not enough space to display the QR code. Try resizing this applet.") : i18nd("klipper", "Creating QR code failed")
    }
}
