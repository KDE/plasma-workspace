/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC

import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.private.clipboard 0.1 as Private
import org.kde.kirigami as Kirigami

PlasmaExtras.Representation {
    id: dialogItem

    implicitWidth: 100 // A friendly initial size
    implicitHeight: 100

    signal requestShowPopup()
    signal requestHidePopup()
    signal requestResizeHeight(real height)

    focus: true
    collapseMarginsHint: true
    header: stack.currentItem.header as Item

    Keys.onEscapePressed: requestHidePopup()
    Keys.forwardTo: [stack.currentItem]

    function editClipboardContent(index: int) {
        if (clipboardMenu.editing || index < 0 || index >= clipboardMenu.view.count) {
            return;
        }
        (clipboardMenu.view as ListView).currentIndex = index;
        ((clipboardMenu.view as ListView).currentItem as ClipboardItemDelegate).edit();
    }

    function showBarcode(index: int): void {
        if (stack.currentItem instanceof BarcodePage || index < 0 || index >= clipboardMenu.view.count) {
            return;
        }
        const view = clipboardMenu.view as ListView;
        view.currentIndex = index;
        (view.currentItem as ClipboardItemDelegate).barcode();
    }

    function showActionMenu(index: int, automaticallyInvoked: bool): void {
        historyModel.starredOnly = 0;
        if (index < 0 || index >= clipboardMenu.view.count) {
            return;
        }
        if (stack.currentItem instanceof ActionMenu) {
            stack.popCurrentItem(QQC.StackView.Immediate);
        }
        const view = clipboardMenu.view as ListView;
        view.currentIndex = index;
        (view.currentItem as ClipboardItemDelegate).triggerAction(automaticallyInvoked);
    }

    function updateContentSize(screenSize: size): void {
        dialogItem.implicitWidth = Math.max(Kirigami.Units.gridUnit * 15, Math.min(screenSize.width / 4, Kirigami.Units.gridUnit * 40));
        dialogItem.implicitHeight = Math.min(screenSize.height / 2, Kirigami.Units.gridUnit * 40);
    }

    Connections {
        target: dialogItem.Window.window
        function onVisibleChanged() {
            if (!dialogItem.Window.window.visible) {
                ((stack.initialItem as Private.ClipboardMenu).view as ListView).positionViewAtBeginning();
            }
            ((stack.initialItem as Private.ClipboardMenu).view as ListView).currentIndex = 0;
        }
    }

    Connections {
        enabled: Private.URLGrabber.enabled
        target: Private.URLGrabber
        function onRequestShowCurrentActionMenu() {
            dialogItem.requestShowPopup();
            dialogItem.showActionMenu(0, true);
        }
    }

    Private.HistoryModel {
        id: historyModel
    }

    QQC.StackView {
        id: stack
        anchors.fill: parent
        initialItem: Private.ClipboardMenu {
            id: clipboardMenu
            expanded: dialogItem.Window.window.visible
            dialogItem: dialogItem
            model: historyModel
            showsClearHistoryButton: true
            barcodeType: "QRCode"

            onRequestHidePopup: dialogItem.requestHidePopup()
            onRequestResizeHeight: height => dialogItem.requestResizeHeight(height + dialogItem.header.height)
        }
    }
}
