/*
    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.breeze.components as BreezeComponents
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.workspace.components as WorkspaceComponents
import org.kde.plasma.workspace.keyboardlayout as Keyboards

/*!
  A footer for the login and lock screens that contains components for e.g. 
  switching keyboard layouts and showing the on-screen keyboard.
*/
RowLayout {
    // Note: The containment masks on the buttons stretch their clickable area to
    // the screen edges, essentially making them adhere to Fitts's law.
    //
    // Due to virtual keyboard button having an icon, buttons may have
    // different heights, so fillHeight is required.

    id: root

    /*!
      Signal emitted when the on-screen keyboard button is clicked.

      This signal is used to notify the login and lock screens that the on-screen
      keyboard is being activated, so they can ensure focus is set on the input field.
    */
    signal oskActivated

    anchors {
        bottom: parent.bottom
        left: parent.left
        right: parent.right
        margins: Kirigami.Units.smallSpacing
    }

    spacing: Kirigami.Units.smallSpacing

    PlasmaComponents.ToolButton {
        id: onScreenKeyboardButton

        text: i18ndc("plasma_shell_org.kde.plasma.desktop", "Button to show/hide virtual keyboard", "Virtual Keyboard")
        icon.name: Keyboards.KWinVirtualKeyboard.visible ? "input-keyboard-virtual-on" : "input-keyboard-virtual-off"

        TapHandler {
            onTapped: (eventPoint, button) => {
                if (button === Qt.NoButton) {
                    // Touchscreen
                    Keyboards.KWinVirtualKeyboard.mode = Keyboards.KWinVirtualKeyboard.NonMouseInput;
                } else if (button === Qt.LeftButton) {
                    Keyboards.KWinVirtualKeyboard.mode = Keyboards.KWinVirtualKeyboard.AnyInput;
                }

                root.oskActivated();
            }
        }

        Layout.fillHeight: true

        containmentMask: Item {
            parent: onScreenKeyboardButton
            anchors.fill: parent
            anchors.leftMargin: -root.anchors.margins
            anchors.bottomMargin: -root.anchors.margins
        }
    }

    PlasmaComponents.ToolButton {
        id: keyboardButton

        Accessible.description: i18ndc("plasma_shell_org.kde.plasma.desktop", "Button to change keyboard layout", "Switch layout")
        icon.name: "input-keyboard"

        WorkspaceComponents.KeyboardLayoutSwitcher {
            id: keyboardLayoutSwitcher

            anchors.fill: parent
            acceptedButtons: Qt.NoButton
        }

        text: keyboardLayoutSwitcher.layoutNames.longName
        onClicked: keyboardLayoutSwitcher.keyboardLayout.switchToNextLayout()

        visible: keyboardLayoutSwitcher.hasMultipleKeyboardLayouts

        Layout.fillHeight: true

        containmentMask: Item {
            parent: keyboardButton
            anchors.fill: parent
            anchors.leftMargin: onScreenKeyboardButton.visible ? 0 : -root.anchors.margins
            anchors.bottomMargin: -root.anchors.margins
        }
    }

    Item {
        Layout.fillWidth: true
    }

    BreezeComponents.Battery {}
}
