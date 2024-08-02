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

    implicitWidth: Math.max(Kirigami.Units.gridUnit * 15, Math.min(Screen.width / 4, Kirigami.Units.gridUnit * 40))
    implicitHeight: Math.min(Screen.height / 2, Kirigami.Units.gridUnit * 40)

    signal requestHidePopup()

    focus: true
    collapseMarginsHint: true
    header: stack.currentItem.header as Item

    Keys.onEscapePressed: requestHidePopup()
    Keys.forwardTo: [stack.currentItem]

    Connections {
        target: dialogItem
        function onRequestHidePopup() {
            ((stack.initialItem as Private.ClipboardMenu).view as ListView).currentIndex = 0;
            ((stack.initialItem as Private.ClipboardMenu).view as ListView).positionViewAtBeginning();
        }
    }

    Private.HistoryModel {
        id: historyModel
    }

    QQC.StackView {
        id: stack
        anchors.fill: parent
        initialItem: Private.ClipboardMenu {
            expanded: dialogItem.Window.window.visible
            dialogItem: dialogItem
            model: historyModel
            showsClearHistoryButton: true
            barcodeType: "QRCode"

            onItemSelected: dialogItem.requestHidePopup()
        }
    }
}
