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

    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.fullRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
        source: Plasmoid.icon
        active: compactMouse.containsMouse
        overlays: root.overlays

        MouseArea {
            id: compactMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: if (!Keyboards.KWinVirtualKeyboard.available) {
                root.action_settings()
            } else if (Keyboards.KWinVirtualKeyboard.visible) {
                Keyboards.KWinVirtualKeyboard.active = false
            } else {
                Keyboards.KWinVirtualKeyboard.enabled = !Keyboards.KWinVirtualKeyboard.enabled
            }
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
            name: "available"
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
            name: "visible"
            when: Keyboards.KWinVirtualKeyboard.available && Keyboards.KWinVirtualKeyboard.visible
            PropertyChanges {
                target: Plasmoid.self
                icon: "arrow-down"
                toolTipSubText: i18n("Virtual Keyboard: visible")
                // It's only relevant in tablet mode
                status: Kirigami.Settings.tabletMode ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
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
