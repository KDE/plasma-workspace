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

    signal requestHidePopup()

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

            onItemSelected: dialogItem.requestHidePopup()
        }
    }
}
