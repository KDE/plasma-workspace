/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3

GridLayout {
    id: toolButtonsLayout

    enum ButtonRole {
        ToggleStar,
        InvokeAction,
        ShowQRCode,
        Edit,
        Remove
    }

    required property PlasmaComponents3.ItemDelegate delegateItem
    required property bool shouldUseOverflowButton

    readonly property ClipboardItemDelegate item: delegateItem as ClipboardItemDelegate

    // Constants for item types (from HistoryItemType enum in historyitem.h)
    readonly property int textItemType: 2  // HistoryItemType::Text = 1 << 1

    readonly property list<var> buttonDefinitions: [
        {
            role: DelegateToolButtons.ButtonRole.InvokeAction,
            icon: "system-run",
            text: i18nd("klipper", "Invoke action"),
            visible: item.model?.hasAction ?? false
        },
        {
            role: DelegateToolButtons.ButtonRole.ShowQRCode,
            icon: "view-barcode-qr",
            text: i18nd("klipper", "Show QR code"),
            visible: item.type === toolButtonsLayout.textItemType
        },
        {
            role: DelegateToolButtons.ButtonRole.Edit,
            icon: "document-edit",
            text: i18nd("klipper", "Edit contents"),
            visible: item.type === toolButtonsLayout.textItemType
        },
        {
            role: DelegateToolButtons.ButtonRole.Remove,
            icon: "edit-delete",
            text: i18nd("klipper", "Remove from history")
        },
        {
            role: DelegateToolButtons.ButtonRole.ToggleStar,
            icon: (item.model?.starred ?? false) ? "starred-symbolic" : "non-starred-symbolic",
            text: (item.model?.starred ?? false) ? i18nd("klipper", "Remove Star") : i18nd("klipper", "Star"),
            enabled: !!item.model
        }
    ]

    readonly property int visibleButtonCount: {
        let count = 0;
        for (let i = 0; i < buttonDefinitions.length; i++) {
            if (buttonDefinitions[i].visible ?? true) {
                count++;
            }
        }
        return count;
    }

    readonly property Item defaultButton: visibleChildren.length > 0 ? visibleChildren[0] : this
    // https://bugreports.qt.io/browse/QTBUG-108821
    readonly property bool hovered: visibleChildren.filter(x => x.hovered).length > 0

    rows: shouldUseOverflowButton ? visibleButtonCount : 1
    columns: shouldUseOverflowButton ? 1 : visibleButtonCount
    rowSpacing: Kirigami.Units.smallSpacing
    columnSpacing: Kirigami.Units.smallSpacing

    function trigger(actionRole: int): void {
        switch (actionRole) {
        case DelegateToolButtons.ButtonRole.ToggleStar:
            if (item.model) {
                // Make sure highlight does not jump where pointer is when using a keyboard
                (delegateItem.ListView.view.parent as ClipboardMenu).hoverBlock.reset();
                (delegateItem.ListView.view.parent as ClipboardMenu).keyboardActivated = true;
                item.model.starred = !(item.model?.starred ?? false);
            }
            break;
        case DelegateToolButtons.ButtonRole.InvokeAction:
            item.triggerAction();
            break;
        case DelegateToolButtons.ButtonRole.ShowQRCode:
            item.barcode();
            break;
        case DelegateToolButtons.ButtonRole.Edit:
            item.edit();
            break;
        case DelegateToolButtons.ButtonRole.Remove:
            item.remove();
            break;
        }
    }

    Repeater {
        id: repeater
        model: toolButtonsLayout.buttonDefinitions

        PlasmaComponents3.ToolButton {
            required property int index
            required property var modelData

            Layout.fillWidth: toolButtonsLayout.shouldUseOverflowButton
            Layout.leftMargin: toolButtonsLayout.shouldUseOverflowButton ? Kirigami.Units.gridUnit : 0
            Layout.rightMargin: toolButtonsLayout.shouldUseOverflowButton ? Kirigami.Units.gridUnit : 0

            display: toolButtonsLayout.shouldUseOverflowButton ? PlasmaComponents3.AbstractButton.TextBesideIcon : PlasmaComponents3.AbstractButton.IconOnly
            text: modelData.text
            icon.name: modelData.icon
            enabled: modelData.enabled ?? true
            visible: modelData.visible ?? true

            KeyNavigation.right: (index === repeater.count - 1 ? toolButtonsLayout.delegateItem.ListView.view : repeater.itemAt(index + 1))
            KeyNavigation.left: (index === 0 ? toolButtonsLayout.delegateItem : repeater.itemAt(index - 1))
            PlasmaComponents3.ToolTip.text: text
            PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
            PlasmaComponents3.ToolTip.visible: hovered || (activeFocus && (focusReason === Qt.TabFocusReason || focusReason === Qt.BacktabFocusReason))
            onClicked: toolButtonsLayout.trigger(modelData.role)
        }
    }
}
