/********************************************************************
This file is part of the KDE project.

Copyright (C) 2015 Martin Gräßlin <mgraesslin@kde.org>

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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents // For ContextMenu
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.prison 1.0 as Prison

ColumnLayout {
    id: barcodeView

    property alias text: barcodeItem.content

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
                PlasmaComponents.MenuItem { }
            }

            PlasmaComponents.ContextMenu {
                id: menu
                visualParent: configureButton
                placement: PlasmaCore.Types.BottomPosedLeftAlignedPopup
                onStatusChanged: {
                    if (status == PlasmaComponents.DialogStatus.Closed) {
                        configureButton.checked = false;
                    }
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
                            checked: Qt.binding(() => {
                                return barcodeItem.barcodeType === item.type;
                            })
                        });
                        menuItem.clicked.connect(() => {
                            barcodeItem.barcodeType = item.type;
                        });
                        menu.addMenuItem(menuItem);
                    });
                }
            }
            PlasmaComponents3.ToolButton {
                id: configureButton
                checkable: true
                icon.name: "configure"
                onClicked: menu.openRelative()

                PlasmaComponents3.ToolTip {
                    text: i18n("Change the barcode type")
                }
            }
        }
    }

    Item {
        Layout.fillWidth: parent
        Layout.fillHeight: parent
        Layout.topMargin: units.smallSpacing

        Prison.Barcode {
            id: barcodeItem
            readonly property bool valid: implicitWidth > 0 && implicitHeight > 0 && implicitWidth <= width && implicitHeight <= height
            anchors.fill: parent
            barcodeType: Prison.Barcode.QRCode
            // Cannot set visible to false as we need it to re-render when changing its size
            opacity: valid ? 1 : 0
        }

        PlasmaComponents3.Label {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: i18n("Creating barcode failed")
            wrapMode: Text.WordWrap
            visible: barcodeItem.implicitWidth === 0 && barcodeItem.implicitHeight === 0
        }

        PlasmaComponents3.Label {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: i18n("The barcode is too large to be displayed")
            wrapMode: Text.WordWrap
            visible: barcodeItem.implicitWidth > barcodeItem.width || barcodeItem.implicitHeight > barcodeItem.height
        }
    }
}
