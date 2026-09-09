/*
 *    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>
 *
 *    SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.workspace.keyboardlayout as Keyboards
import org.kde.kcmutils // KCMLauncher
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    readonly property string title: i18nd("plasma_applet_org.kde.plasma.manageinputmethod", "On-Screen Keyboard")

    Plasmoid.onActivated: {
        if (state === "unsupported") {
            // When the current client doesn't support input methods, we can force
            // the display of the on-screen keyboard so it emulates a hardware keyboard instead.
            Keyboards.KWinVirtualKeyboard.forceActivate();
        } else if (state === "visible") {
            Keyboards.KWinVirtualKeyboard.active = false;
        } else {
            settingsAction.trigger();
        }
    }
    preferredRepresentation: fullRepresentation
    fullRepresentation: Kirigami.Icon {
        activeFocusOnTab: true
        source: Plasmoid.icon
        active: compactMouse.containsMouse

        Keys.onPressed: event => {
            switch (event.key) {
            case Qt.Key_Space:
            case Qt.Key_Enter:
            case Qt.Key_Return:
            case Qt.Key_Select:
                Plasmoid.activated();
                break;
            }
        }
        Accessible.name: Plasmoid.title
        Accessible.description: root.toolTipSubText
        Accessible.role: Accessible.Button

        MouseArea {
            id: compactMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: Plasmoid.activated()
        }
        Kirigami.Icon {
            anchors {
                right: parent.right
                bottom: parent.bottom
            }
            width: Kirigami.Units.iconSizes.small / 2
            height: width
            visible: root.state === "unavailable"
            source: visible ? "emblem-unavailable" : ""
        }
    }

    PlasmaCore.ActionGroup {
        id: oskGroup
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: i18nc("@action:tray context menu", "Disabled")
            icon.name: "edit-none-symbolic"
            enabled: Keyboards.KWinVirtualKeyboard.available
            actionGroup: oskGroup
            checkable: true
            checked: Keyboards.KWinVirtualKeyboard.mode === Keyboards.KWinVirtualKeyboard.Never
            onTriggered: {
                Keyboards.KWinVirtualKeyboard.mode = Keyboards.KWinVirtualKeyboard.Never
            }
        },
        PlasmaCore.Action {
            text: i18nc("@action:tray context menu", "Touch and Tablet")
            icon.name: "input-touchscreen-symbolic"
            enabled: Keyboards.KWinVirtualKeyboard.available
            actionGroup: oskGroup
            checkable: true
            checked: Keyboards.KWinVirtualKeyboard.mode === Keyboards.KWinVirtualKeyboard.NonMouseInput
            onTriggered: {
                Keyboards.KWinVirtualKeyboard.mode = Keyboards.KWinVirtualKeyboard.NonMouseInput
            }
        },
        PlasmaCore.Action {
            text: i18nc("@action:tray context menu", "Touch, Tablet, Mouse, and Touchpad")
            icon.name: "input-mouse-symbolic"
            enabled: Keyboards.KWinVirtualKeyboard.available
            actionGroup: oskGroup
            checkable: true
            checked: Keyboards.KWinVirtualKeyboard.mode === Keyboards.KWinVirtualKeyboard.AnyInput
            onTriggered: {
                Keyboards.KWinVirtualKeyboard.mode = Keyboards.KWinVirtualKeyboard.AnyInput
            }
        },
        PlasmaCore.Action {
            isSeparator: true
        },
    ]

    PlasmaCore.Action {
        id: settingsAction
        text: i18ndc("plasma_applet_org.kde.plasma.manageinputmethod", "Opens the system settings module", "Configure On-Screen Keyboards…")
        icon.name: "settings-configure"
        onTriggered: KCMLauncher.openSystemSettings("kcm_virtualkeyboard")
    }
    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", settingsAction)
    }

    property bool inEmbeddedContainment: Plasmoid.containment.containmentType === PlasmaCore.Containment.CustomEmbedded

    states: [
        State {
            name: "unavailable"
            when: !Keyboards.KWinVirtualKeyboard.available
            PropertyChanges {
                root.Plasmoid.icon: "input-keyboard-virtual-off-symbolic"
                root.Plasmoid.status: root.inEmbeddedContainment ? PlasmaCore.Types.HiddenStatus : PlasmaCore.Types.PassiveStatus
                root.toolTipSubText: i18ndc("plasma_applet_org.kde.plasma.manageinputmethod", "@info:tooltip", "On-Screen Keyboard: unavailable\nClick or tap to configure on-screen keyboard settings")
            }
        },
        State {
            name: "disabled"
            when: Keyboards.KWinVirtualKeyboard.available && Keyboards.KWinVirtualKeyboard.mode === Keyboards.KWinVirtualKeyboard.Never && !Keyboards.KWinVirtualKeyboard.visible
            PropertyChanges {
                root.Plasmoid.icon: "input-keyboard-virtual-off-symbolic"
                root.Plasmoid.status: PlasmaCore.Types.PassiveStatus
                root.toolTipMainText: root.title
                root.toolTipSubText: i18ndc("plasma_applet_org.kde.plasma.manageinputmethod", "@info:tooltip", "Configured not to show automatically\nClick or tap to configure on-screen keyboard settings")
            }
        },
        State {
            id: unsupportedState
            name: "unsupported"
            when: Keyboards.KWinVirtualKeyboard.available && !Keyboards.KWinVirtualKeyboard.activeClientSupportsTextInput && !Keyboards.KWinVirtualKeyboard.visible
            // When the current client doesn't support input methods, we can force
            // the display of the on-screen keyboard so it emulates a hardware keyboard instead
            PropertyChanges {
                root.Plasmoid.icon: "input-keyboard-virtual-show-symbolic"
                root.Plasmoid.status: Keyboards.KWinVirtualKeyboard.willShowOnActive ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
                root.toolTipMainText: root.title
                root.toolTipSubText: i18ndc("plasma_applet_org.kde.plasma.manageinputmethod", "@info:tooltip Hardware emulation mode means it acts like a regular dumb keyboard (no input-method features like preedit/word suggestions/etc)", "No supported text text field is currently active\nClick or tap to show in hardware keyboard emulation mode.")
            }
        },
        State {
            name: "visible"
            when: Keyboards.KWinVirtualKeyboard.available && Keyboards.KWinVirtualKeyboard.visible
            PropertyChanges {
                root.Plasmoid.icon: "input-keyboard-virtual-hide-symbolic"
                // Because the keyboard can become visible with a touch input when
                // while not explicitly in Tablet Mode
                root.Plasmoid.status: !Keyboards.KWinVirtualKeyboard.activeClientSupportsTextInput ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
                root.toolTipMainText: i18ndc("plasma_applet_org.kde.plasma.manageinputmethod", "@info:tooltip", "On-Screen Keyboard is visible\nClick or tap to hide the on-screen keyboard.")
            }
        },
        State {
            name: "touchOnly"
            when: Keyboards.KWinVirtualKeyboard.available && Keyboards.KWinVirtualKeyboard.mode === Keyboards.KWinVirtualKeyboard.NonMouseInput && !Keyboards.KWinVirtualKeyboard.visible
            PropertyChanges {
                root.Plasmoid.icon: "input-keyboard-virtual-off-symbolic"
                root.Plasmoid.status: PlasmaCore.Types.PassiveStatus
                root.toolTipMainText: root.title
                root.toolTipSubText: i18ndc("plasma_applet_org.kde.plasma.manageinputmethod", "@info:tooltip", "Shows automatically when tapping a text field\nTap to configure on-screen keyboard settings")
            }
        },
        State {
            name: "alwaysOn"
            when: Keyboards.KWinVirtualKeyboard.available && Keyboards.KWinVirtualKeyboard.mode === Keyboards.KWinVirtualKeyboard.AnyInput && !Keyboards.KWinVirtualKeyboard.visible
            PropertyChanges {
                root.Plasmoid.icon: "input-keyboard-virtual-off-symbolic"
                root.Plasmoid.status: PlasmaCore.Types.PassiveStatus
                root.toolTipMainText: root.title
                root.toolTipSubText: i18ndc("plasma_applet_org.kde.plasma.manageinputmethod", "@info:tooltip", "Shows automatically when interacting with a text field\nClick or tap to configure on-screen keyboard settings")
            }
        }
    ]
}
