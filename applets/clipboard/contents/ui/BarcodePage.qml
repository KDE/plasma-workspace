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

Item {
    id: barcodeView

    readonly property bool valid: barcodeItem.implicitWidth > 0 && barcodeItem.implicitHeight > 0
    readonly property bool fit: barcodeItem.implicitWidth <= barcodeItem.width && barcodeItem.implicitHeight <= barcodeItem.height
    property alias text: barcodeItem.content

    readonly property var barcodeMap: [
        {text: i18n("QR Code"), type: Prison.Barcode.QRCode, code: "QRCode"},
        {text: i18n("Data Matrix"), type: Prison.Barcode.DataMatrix, code: "DataMatrix"},
        {text: i18nc("Aztec barcode", "Aztec"), type: Prison.Barcode.Aztec, code: "Aztec"},
        {text: i18n("Code 39"), type: Prison.Barcode.Code39, code: "Code39"},
        {text: i18n("Code 93"), type: Prison.Barcode.Code93, code: "Code93"},
        {text: i18n("Code 128"), type: Prison.Barcode.Code128, code: "Code128"}
    ]

    Keys.onPressed: event => {
        if (event.key == Qt.Key_Escape) {
            stack.pop()
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
                onClicked: stack.pop()
            }

            PlasmaComponents3.Menu {
                id: menu
            }

            Instantiator {
                id: menuItemInstantiator
                active: main.expanded && menu.opened
                asynchronous: true
                delegate: PlasmaComponents3.MenuItem {
                    text: modelData.text
                    checkable: true
                    autoExclusive: true
                    checked: barcodeItem.barcodeType === modelData.type

                    onClicked: {
                        barcodeItem.barcodeType = modelData.type;
                        Plasmoid.configuration.barcodeType = modelData.code;
                    }
                }
                model: barcodeView.barcodeMap

                onObjectAdded: (index, object) => menu.insertItem(index, object)
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
                    text: parent.text
                }
            }
        }
    }

    Prison.Barcode {
        id: barcodeItem
        anchors.fill: parent
        anchors.topMargin: Kirigami.Units.smallSpacing
        barcodeType: barcodeView.barcodeMap.find(data => data.code === Plasmoid.configuration.barcodeType)?.type ?? barcodeView.barcodeMap[0].type
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
        text: barcodeView.valid ? i18n("There is not enough space to display the QR code. Try resizing this applet.") : i18n("Creating QR code failed")
    }
}
