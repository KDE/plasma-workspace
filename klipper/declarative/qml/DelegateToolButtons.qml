/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts 1.1

import org.kde.kirigami as Kirigami
import org.kde.plasma.components 3.0 as PlasmaComponents3

RowLayout {
    id: toolButtonsLayout

    // https://bugreports.qt.io/browse/QTBUG-108821
    readonly property Item defaultButton: visibleChildren.length > 0 ? visibleChildren[0] : this
    readonly property bool hovered: !menuButton.visible && visibleChildren.filter(x => x.hovered).length > 0
    readonly property list<string> actionIcons: ["system-run", "view-barcode-qr", "document-edit", "edit-delete"]
    readonly property list<string> actionNames: [
        i18nd("klipper", "Invoke action"),
        i18nd("klipper", "Show QR code"),
        i18nd("klipper", "Edit contents"),
        i18nd("klipper", "Remove from history")
    ]

    function trigger(actionIndex: int): void {
        switch (actionIndex) {
        case 0:
            menuItem.triggerAction();
            break;
        case 1:
            menuItem.barcode();
            break;
        case 2:
            menuItem.edit();
            break;
        case 3:
            menuItem.remove();
            break;
        }
    }

    Repeater {
        id: repeater
        model: !menuButton.visible ? 4 : 0
        PlasmaComponents3.ToolButton {
            required property int index
            visible: index != 2 || menuItem.type === 0
            focus: index === 0
            display: PlasmaComponents3.AbstractButton.IconOnly
            text: toolButtonsLayout.actionNames[index]
            icon.name: toolButtonsLayout.actionIcons[index]
            KeyNavigation.right: index === repeater.count - 1 ? this : repeater.itemAt(index + 1)
            PlasmaComponents3.ToolTip.text: text
            PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
            PlasmaComponents3.ToolTip.visible: hovered || (activeFocus && (focusReason === Qt.TabFocusReason || focusReason === Qt.BacktabFocusReason))
            onClicked: toolButtonsLayout.trigger(index)
        }
    }

    PlasmaComponents3.Menu {
        id: menu
        Repeater {
            model: menu.opened ? 4 : 0
            PlasmaComponents3.MenuItem {
                required property int index
                visible: index != 2 || menuItem.type === 0
                height: index != 2 || menuItem.type === 0 ? undefined : 0
                text: toolButtonsLayout.actionNames[index]
                icon.name: toolButtonsLayout.actionIcons[index]
                onClicked: toolButtonsLayout.trigger(index)
            }
        }
    }

    PlasmaComponents3.ToolButton {
        id: menuButton

        visible: Kirigami.Settings.tabletMode || Kirigami.Settings.hasTransientTouchInput
        checked: menu.opened
        icon.name: "overflow-menu"
        PlasmaComponents3.ToolTip.text: i18ndc("klipper", "@action:button", "More actions")
        PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
        PlasmaComponents3.ToolTip.visible: pressed

        onClicked: if (checked) {
            menu.close();
        } else {
            menuItem.ListView.view.currentIndex = menuItem.index;
            menu.popup(menuButton);
        }
    }
}
