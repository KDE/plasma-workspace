/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.components 3.0 as PlasmaComponents3

GridLayout {
    id: toolButtonsLayout

    enum ButtonRole {
        InvokeAction,
        ShowQRCode,
        Edit,
        Remove
    }

    readonly property Item defaultButton: visibleChildren.length > 0 ? visibleChildren[0] : this
    // https://bugreports.qt.io/browse/QTBUG-108821
    readonly property bool hovered: visibleChildren.filter(x => x.hovered).length > 0
    readonly property list<string> actionIcons: ["system-run", "view-barcode-qr", "document-edit", "edit-delete"]
    readonly property list<string> actionNames: [
        i18nd("klipper", "Invoke action"),
        i18nd("klipper", "Show QR code"),
        i18nd("klipper", "Edit contents"),
        i18nd("klipper", "Remove from history")
    ]

    required property PlasmaComponents3.ItemDelegate menuItem
    required property bool shouldUseOverflowButton

    rows: shouldUseOverflowButton ? (actionNames.length - (menuItem.type === 0 ? 0 : 1)) : 1
    columns: shouldUseOverflowButton ? 1 : (actionNames.length - (menuItem.type === 0 ? 0 : 1))
    rowSpacing: Kirigami.Units.smallSpacing
    columnSpacing: Kirigami.Units.smallSpacing

    function trigger(actionIndex: int): void {
        switch (actionIndex) {
        case DelegateToolButtons.ButtonRole.InvokeAction:
            menuItem.triggerAction();
            break;
        case DelegateToolButtons.ButtonRole.ShowQRCode:
            menuItem.barcode();
            break;
        case DelegateToolButtons.ButtonRole.Edit:
            menuItem.edit();
            break;
        case DelegateToolButtons.ButtonRole.Remove:
            menuItem.remove();
            break;
        }
    }

    Repeater {
        id: repeater
        model: 4
        PlasmaComponents3.ToolButton {
            required property int index
            Layout.fillWidth: toolButtonsLayout.shouldUseOverflowButton
            Layout.leftMargin: toolButtonsLayout.shouldUseOverflowButton ? Kirigami.Units.gridUnit : 0
            Layout.rightMargin: toolButtonsLayout.shouldUseOverflowButton ? Kirigami.Units.gridUnit : 0
            visible: index != DelegateToolButtons.ButtonRole.Edit || toolButtonsLayout.menuItem.type === 0
            display: toolButtonsLayout.shouldUseOverflowButton ? PlasmaComponents3.AbstractButton.TextBesideIcon : PlasmaComponents3.AbstractButton.IconOnly
            text: toolButtonsLayout.actionNames[index]
            icon.name: toolButtonsLayout.actionIcons[index]
            KeyNavigation.right: (index === repeater.count - 1 ? this : repeater.itemAt(index + 1)) as PlasmaComponents3.ToolButton
            PlasmaComponents3.ToolTip.text: text
            PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
            PlasmaComponents3.ToolTip.visible: hovered || (activeFocus && (focusReason === Qt.TabFocusReason || focusReason === Qt.BacktabFocusReason))
            onClicked: toolButtonsLayout.trigger(index)
        }
    }
}
