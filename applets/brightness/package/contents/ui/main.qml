/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021-2022 ivan tkachenko <me@ratijas.tk>
    SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.coreaddons as KCoreAddons
import org.kde.kcmutils // KCMLauncher
import org.kde.config // KAuthorized
import org.kde.notification
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

import org.kde.plasma.private.brightnesscontrolplugin

PlasmoidItem {
    id: brightnessAndColorControl

    readonly property bool inPanel: (Plasmoid.location === PlasmaCore.Types.TopEdge
        || Plasmoid.location === PlasmaCore.Types.RightEdge
        || Plasmoid.location === PlasmaCore.Types.BottomEdge
        || Plasmoid.location === PlasmaCore.Types.LeftEdge)

    NightLightMonitor {
        id: nightLightMonitor
    }
    NightLightInhibitor {
        id: nightLightInhibitor
    }
    ScreenBrightnessControl {
        id: screenBrightnessControl
        isSilent: brightnessAndColorControl.expanded
    }
    KeyboardBrightnessControl {
        id: keyboardBrightnessControl
        isSilent: brightnessAndColorControl.expanded
    }
    KeyboardColorControl {
        id: keyboardColorControl
    }

    property bool isNightLightActive: nightLightMonitor.running && nightLightMonitor.currentTemperature != 6500
    property bool isNightLightInhibited: nightLightInhibitor.state == NightLightInhibitor.Inhibiting || nightLightInhibitor.state == NightLightInhibitor.Inhibited && nightLightMonitor.targetTemperature != 6500
    property int screenBrightnessPercent: screenBrightnessControl.brightnessMax ? Math.round(100 * screenBrightnessControl.brightness / screenBrightnessControl.brightnessMax) : 0
    property int keyboardBrightnessPercent: keyboardBrightnessControl.brightnessMax ? Math.round(100 * keyboardBrightnessControl.brightness / keyboardBrightnessControl.brightnessMax) : 0

    function symbolicizeIconName(iconName) {
        const symbolicSuffix = "-symbolic";
        if (iconName.endsWith(symbolicSuffix)) {
            return iconName;
        }

        return iconName + symbolicSuffix;
    }

    switchWidth: Kirigami.Units.gridUnit * 10
    switchHeight: Kirigami.Units.gridUnit * 10

    Plasmoid.title: i18n("Brightness and Color")

    LayoutMirroring.enabled: Qt.application.layoutDirection == Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Plasmoid.status: {
        return screenBrightnessControl.isBrightnessAvailable || keyboardBrightnessControl.isBrightnessAvailable || isNightLightActive || isNightLightInhibited ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus;
    }

    toolTipMainText: {
        const parts = [];
        if (screenBrightnessControl.isBrightnessAvailable) {
            parts.push(i18n("Screen brightness at %1%", screenBrightnessPercent));
        }
        if (keyboardBrightnessControl.isBrightnessAvailable) {
            parts.push(i18n("Keyboard brightness at %1%", keyboardBrightnessPercent));
        }
        if (nightLightMonitor.enabled) {
            if (!nightLightMonitor.running) {
                parts.push(i18nc("Status", "Night Light off"));
            } else if (nightLightMonitor.currentTemperature != 6500) {
                parts.push(i18nc("Status; placeholder is a temperature", "Night Light at %1K", nightLightMonitor.currentTemperature));
            }
        }

        return parts.join("\n");
    }

    toolTipSubText: {
        const parts = [];
        if (screenBrightnessControl.isBrightnessAvailable) {
            parts.push(i18n("Scroll to adjust screen brightness"));
        }
        if (nightLightMonitor.enabled) {
            parts.push(i18n("Middle-click to toggle Night Light"));
        }
        return parts.join("\n");
    }

    Plasmoid.icon: {
        let iconName = "brightness-high";

        if (nightLightMonitor.enabled) {
            if (!nightLightMonitor.running) {
                iconName = "redshift-status-off";
            } else if (nightLightMonitor.currentTemperature != 6500) {
                if (nightLightMonitor.daylight) {
                    iconName = "redshift-status-day";
                } else {
                    iconName = "redshift-status-on";
                }
            }
        }

        if (inPanel) {
            return symbolicizeIconName(iconName);
        }

        return iconName;
    }

    compactRepresentation: CompactRepresentation {

        onWheel: wheel => {
            if (!screenBrightnessControl.isBrightnessAvailable) {
                return;
            }
            const delta = (wheel.inverted ? -1 : 1) * (wheel.angleDelta.y ? wheel.angleDelta.y : -wheel.angleDelta.x);

            const brightnessMax = screenBrightnessControl.brightnessMax
            const brightnessMin = Plasmoid.configuration.allowMinScreenBrightness ? 0 : 1
            const stepSize = Math.max(1, brightnessMax / 20)

            let newBrightness;
            if (Math.abs(delta) < 120) {
                // Touchpad scrolling
                brightnessError += delta * stepSize / 120;
                const change = Math.round(brightnessError);
                brightnessError -= change;
                newBrightness = screenBrightnessControl.brightness + change;
            } else if (wheel.modifiers & Qt.ShiftModifier) {
                newBrightness = Math.round((Math.round(screenBrightnessControl.brightness * 100 / brightnessMax) + delta/120) / 100 * maximumBrightness)
            } else {
                // Discrete/wheel scrolling
                newBrightness = Math.round(screenBrightnessControl.brightness/stepSize + delta/120) * stepSize;
            }
            screenBrightnessControl.brightness = Math.max(brightnessMin, Math.min(brightnessMax, newBrightness));
        }

        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        property bool wasExpanded: false
        onPressed: wasExpanded = brightnessAndColorControl.expanded
        onClicked: mouse => {
            if (mouse.button == Qt.MiddleButton) {
                toggleNightLightInhibition();
            } else {
                brightnessAndColorControl.expanded = !wasExpanded;
            }
        }

        function toggleNightLightInhibition() {
            if (!nightLightMonitor.available) {
                return;
            }
            switch (nightLightInhibitor.state) {
            case NightLightInhibitor.Inhibiting:
            case NightLightInhibitor.Inhibited:
                nightLightInhibitor.uninhibit();
                break;
            case NightLightInhibitor.Uninhibiting:
            case NightLightInhibitor.Uninhibited:
                nightLightInhibitor.inhibit();
                break;
            }
        }
    }

    fullRepresentation: PopupDialog {
        id: dialogItem

        readonly property var appletInterface: brightnessAndColorControl
        readonly property var configuration: Plasmoid.configuration

        Layout.minimumWidth: Kirigami.Units.gridUnit * 10
        Layout.maximumWidth: Kirigami.Units.gridUnit * 80
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20

        Layout.minimumHeight: Kirigami.Units.gridUnit * 10
        Layout.maximumHeight: Kirigami.Units.gridUnit * 40
        Layout.preferredHeight: implicitHeight

    } // todo

    Plasmoid.contextualActions: [
        // By default don't allow the UI to turn off the screen
        // Please see https://git.reviewboard.kde.org/r/122505/ for more information
        PlasmaCore.Action {
            text: i18n("Allow setting display brightness to minimum")
            icon.name: "video-display-brightness"
            visible: screenBrightnessControl.isBrightnessAvailable
            checkable: true
            checked: Plasmoid.configuration.allowMinScreenBrightness 
            onTriggered: checked => {
                Plasmoid.configuration.allowMinScreenBrightness = checked
            }
        },

        PlasmaCore.Action {
            id: configureNightLight
            icon.name: "configure"
            text: i18nc("@action:inmenu", "Configure Night Light…")
            visible: KAuthorized.authorize("kcm_nightlight")
            onTriggered: KCMLauncher.openSystemSettings("kcm_nightlight")
        }
    ]

}
