/*
 *    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *    SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.workspace.keyboardlayout 1.0 as Keyboards
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.5 as Kirigami // For Settings.tabletMode

Item {
    id: root
    property var overlays: []

    Plasmoid.onActivated: {
        if (!Keyboards.KWinVirtualKeyboard.available) {
            root.action_settings()
        } else if (unsupportedState.when) {
            Keyboards.KWinVirtualKeyboard.forceActivate()
        } else if (Keyboards.KWinVirtualKeyboard.visible) {
            Keyboards.KWinVirtualKeyboard.active = false
        } else {
            Keyboards.KWinVirtualKeyboard.enabled = !Keyboards.KWinVirtualKeyboard.enabled
        }
    }
    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.fullRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
        activeFocusOnTab: true
        source: Plasmoid.icon
        active: compactMouse.containsMouse
        overlays: root.overlays

        Keys.onPressed: {
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
        Accessible.description: Plasmoid.toolTipSubText
        Accessible.role: Accessible.Button

        MouseArea {
            id: compactMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: Plasmoid.activated()
        }
    }

    Component.onCompleted: {
        Plasmoid.setAction("settings", i18nc("Opens the system settings module", "Configure Virtual Keyboards..."),
                               "settings-configure")
    }

    function action_settings() {
        KCMShell.openSystemSettings("kcm_virtualkeyboard");
    }

    states: [
        State {
            name: "unavailable"
            when: !Keyboards.KWinVirtualKeyboard.available
            PropertyChanges {
                target: Plasmoid.self
                icon: "input-keyboard-virtual-off"
                toolTipSubText: i18n("Virtual Keyboard: unavailable")
                status: PlasmaCore.Types.HiddenStatus
            }
            PropertyChanges { target: root; overlays: [ "emblem-unavailable" ] }
        },
        State {
            name: "disabled"
            when: Keyboards.KWinVirtualKeyboard.available && !Keyboards.KWinVirtualKeyboard.enabled
            PropertyChanges {
                target: Plasmoid.self
                icon: "input-keyboard-virtual-off"
                toolTipSubText: i18n("Virtual Keyboard: disabled")
                status: PlasmaCore.Types.ActiveStatus
            }
            PropertyChanges { target: root; overlays: [] }
        },
        State {
            id: unsupportedState
            name: "unsupported"
            when: Keyboards.KWinVirtualKeyboard.available && !Keyboards.KWinVirtualKeyboard.activeClientSupportsTextInput
            // When the current client doesn't support input methods, we can force
            // the display of the virtual keyboard so it emulates a hardware keyboard instead
            PropertyChanges {
                target: Plasmoid.self
                icon: "arrow-up"
                toolTipSubText: i18n("Show Virtual Keyboard")
                status: Kirigami.Settings.tabletMode ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
            }
            PropertyChanges { target: root; overlays: [] }
        },
        State {
            name: "visible"
            when: Keyboards.KWinVirtualKeyboard.available && Keyboards.KWinVirtualKeyboard.visible
            PropertyChanges {
                target: Plasmoid.self
                icon: "arrow-down"
                toolTipSubText: i18n("Virtual Keyboard: visible")
                // Because the keyboard can become visible with a touch input when
                // while not explicitly in Touch Mode
                status: Kirigami.Settings.hasTransientTouchInput ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
            }
            PropertyChanges { target: root; overlays: [] }
        },
        State {
            name: "idle"
            when: Keyboards.KWinVirtualKeyboard.available && Keyboards.KWinVirtualKeyboard.enabled && !Keyboards.KWinVirtualKeyboard.visible
            PropertyChanges {
                target: Plasmoid.self
                icon: "input-keyboard-virtual-on"
                toolTipSubText: i18n("Virtual Keyboard: enabled")
                // It's only relevant in tablet mode
                status: Kirigami.Settings.tabletMode ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
            }
            PropertyChanges { target: root; overlays: [] }
        }
    ]
}
