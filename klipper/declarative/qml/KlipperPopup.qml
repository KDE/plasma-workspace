/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Window

import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.private.clipboard as Private
import org.kde.kirigami as Kirigami

PlasmaExtras.Representation {
    id: root

    implicitWidth: 100 // A friendly initial size
    implicitHeight: 100

    signal requestHidePopup()

    focus: true
    collapseMarginsHint: true
    header: ((stack.currentItem as Private.ClipboardMenu)?.header ?? (stack.currentItem as BarcodePage)?.header) as Item

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

    function updateContentSize(screenSize: size): void {
        root.implicitWidth = Math.max(Kirigami.Units.gridUnit * 15, Math.min(screenSize.width / 4, Kirigami.Units.gridUnit * 40));
        root.implicitHeight = Math.min(screenSize.height / 2, Kirigami.Units.gridUnit * 40);
    }

    Connections {
        target: root.Window.window
        function onVisibleChanged() {
            if (!root.Window.window.visible) {
                ((stack.initialItem as Private.ClipboardMenu).view as ListView).positionViewAtBeginning();
            } else {
                clipboardMenu.clearFilter();
            }
            ((stack.initialItem as Private.ClipboardMenu).view as ListView).currentIndex = 0;
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
            expanded: root.Window.window.visible
            dialogItem: root
            model: historyModel
            showsClearHistoryButton: true
            keyboardActivated: true
            barcodeType: "QRCode"

            onItemSelected: root.requestHidePopup()
        }
    }
}
