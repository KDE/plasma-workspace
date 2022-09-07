/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents

import org.kde.plasma.workspace.keyboardlayout 1.0 as KWinKeyboardLayouts

PlasmaComponents.ToolButton {
    id: root

    property int currentIndex: root.isWayland ? waylandKeyboardLayout.layout : keyboard.currentLayout

    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Keyboard Layout: %1", instantiator.model[currentIndex].shortName)
    visible: menu.count > 1

    readonly property bool isWayland: Qt.platform.pluginName.includes("wayland")
    KWinKeyboardLayouts.KeyboardLayout {
        // If you are trying to figure out why you don't get any layouts,
        // make sure some are listed in /var/lib/sddm/.config/kxkbrc
        id: waylandKeyboardLayout
        layout: root.currentIndex
    }

    checkable: true
    checked: menu.opened
    onToggled: {
        if (checked) {
            menu.popup(root, 0, 0)
        } else {
            menu.dismiss()
        }
    }

    signal keyboardLayoutChanged()

    PlasmaComponents.Menu {
        id: menu
        PlasmaCore.ColorScope.colorGroup: PlasmaCore.Theme.NormalColorGroup
        PlasmaCore.ColorScope.inherit: false

        Instantiator {
            id: instantiator
            model: root.isWayland ? waylandKeyboardLayout.layoutsList : keyboard.layouts
            onObjectAdded: menu.insertItem(index, object)
            onObjectRemoved: menu.removeItem(object)
            delegate: PlasmaComponents.MenuItem {
                text: modelData.longName
                readonly property string shortName: modelData.shortName
                onTriggered: {
                    if (root.isWayland) {
                        waylandKeyboardLayout.layout = model.index
                    } else {
                        keyboard.currentLayout = model.index
                    }

                    root.keyboardLayoutChanged()
                }
            }
        }
    }
}
