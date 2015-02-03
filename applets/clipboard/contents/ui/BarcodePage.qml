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
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0

ColumnLayout {
    id: barcodeView

    property var uuid: ""
    property int barcodeType: 0

    function show(uuid) {
        barcodeView.uuid = uuid;
        barcodePreview.image = undefined;
        barcodePreview.busy = true;
        var service = clipboardSource.serviceForSource(uuid)
        var operation = service.operationDescription("barcode");
        operation.width = barcodePreview.width;
        operation.height = barcodePreview.height;
        operation.barcodeType = barcodeView.barcodeType;
        var serviceJob = service.startOperationCall(operation);
        serviceJob.finished.connect(function (job) {
            if (!job.error) {
                barcodePreview.image = job.result;
                barcodePreview.busy = false;
            }
        });
    }

    RowLayout {
        Layout.fillWidth: true
        PlasmaComponents.Button {
            Layout.fillWidth: true
            iconSource: "go-previous-view"
            text: i18n("Return to Clipboard")
            onClicked: stack.pop()
        }
        PlasmaComponents.ContextMenu {
            id: menu
            visualParent: configureButton
            function change(type) {
                barcodeView.barcodeType = type;
                barcodeView.show(barcodeView.uuid);
            }
            PlasmaComponents.MenuItem {
                text: i18n("QR Code")
                checkable: true
                checked: barcodeView.barcodeType == 0
                onClicked: menu.change(0)
            }
            PlasmaComponents.MenuItem {
                text: i18n("Data Matrix")
                checkable: true
                checked: barcodeView.barcodeType == 1
                onClicked: menu.change(1)
            }
            PlasmaComponents.MenuItem {
                text: i18n("Code 39")
                checkable: true
                checked: barcodeView.barcodeType == 2
                onClicked: menu.change(2)
            }
            PlasmaComponents.MenuItem {
                text: i18n("Code 93")
                checkable: true
                checked: barcodeView.barcodeType == 3
                onClicked: menu.change(3)
            }
        }
        PlasmaComponents.ToolButton {
            id: configureButton
            iconSource: "configure"
            tooltip: i18n("Change the barcode type")
            onClicked: menu.open(0, configureButton.height)
        }
    }
    QImageItem {
        id: barcodePreview
        property alias busy: busyIndicator.visible
        fillMode: QImageItem.PreserveAspectFit
        Layout.fillWidth: true
        Layout.fillHeight: true
        onWidthChanged: barcodeView.show(barcodeView.uuid)
        onHeightChanged: barcodeView.show(barcodeView.uuid)
        PlasmaComponents.BusyIndicator {
            id: busyIndicator
            anchors.centerIn: parent
        }
        PlasmaComponents.Label {
            anchors.centerIn: parent
            text: i18n("Creating barcode failed")
            visible: !barcodePreview.busy && barcodePreview.null
        }
    }
}
