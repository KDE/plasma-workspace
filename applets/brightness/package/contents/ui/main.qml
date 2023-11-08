/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021-2022 ivan tkachenko <me@ratijas.tk>
    SPDX-FileCopyrightText: 2023 Natalie Clarius <natalie.clarius@kde.org

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.coreaddons as KCoreAddons
import org.kde.kcmutils // KCMLauncher
import org.kde.config // KAuthorized
import org.kde.notification
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

import org.kde.plasma.private.brightnesscontrolplugin

import "logic.js" as Logic

PlasmoidItem {
    id: brightnesscontrol

    property QtObject pmSource: P5Support.DataSource {
        id: pmSource
        engine: "powermanagement"
        connectedSources: sources
        onSourceAdded: source => {
            disconnectSource(source);
            connectSource(source);
        }
        onSourceRemoved: source => {
            disconnectSource(source);
        }
        onDataChanged: {
            Logic.updateBrightness(brightnesscontrol, pmSource);
        }
    }
    property QtObject updateScreenBrightnessJob
    property QtObject updateKeyboardBrightnessJob

    readonly property bool isScreenBrightnessAvailable: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Screen Brightness Available"] ? true : false
    readonly property bool isKeyboardBrightnessAvailable: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Keyboard Brightness Available"] ? true : false
    readonly property bool isBrightnessAvailable: isScreenBrightnessAvailable || isKeyboardBrightnessAvailable
    readonly property int maximumScreenBrightness: pmSource.data["PowerDevil"] ? pmSource.data["PowerDevil"]["Maximum Screen Brightness"] || 0 : 0
    readonly property int maximumKeyboardBrightness: pmSource.data["PowerDevil"] ? pmSource.data["PowerDevil"]["Maximum Keyboard Brightness"] || 0 : 0

    readonly property bool inPanel: (Plasmoid.location === PlasmaCore.Types.TopEdge
        || Plasmoid.location === PlasmaCore.Types.RightEdge
        || Plasmoid.location === PlasmaCore.Types.BottomEdge
        || Plasmoid.location === PlasmaCore.Types.LeftEdge)

    property bool disableBrightnessUpdate: true
    property int screenBrightness
    property int keyboardBrightness
    property int screenBrightnessPercent: maximumScreenBrightness ? Math.round(100 * screenBrightness / maximumScreenBrightness) : 0
    property int keyboardBrightnessPercent: maximumKeyboardBrightness ? Math.round(100 * keyboardBrightness / maximumKeyboardBrightness) : 0

    NightColorMonitor {
        id: nightColorMonitor
    }
    NightColorInhibitor {
        id: nightColorInhibitor
    }
    property bool isNightColorActive: nightColorMonitor.running && nightColorMonitor.currentTemperature != 6500

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
        return isScreenBrightnessAvailable || isKeyboardBrightnessAvailable || isNightColorActive ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus;
    }

    toolTipMainText: {
        const parts = [];
        if (isScreenBrightnessAvailable) {
            parts.push(i18n("Screen brightness at %1%", screenBrightnessPercent));
        }
        if (isKeyboardBrightnessAvailable) {
            parts.push(i18n("Keyboard brightness at %1%", keyboardBrightnessPercent));
        }
        if (nightColorMonitor.enabled) {
            if (!nightColorMonitor.running) {
                parts.push(i18nc("Status", "Night Light off"));
            } else if (nightColorMonitor.currentTemperature != 6500) {
                parts.push(i18nc("Status; placeholder is a temperature", "Night Light at %1K", nightColorMonitor.currentTemperature));
            }
        }
        
        return parts.join("\n");
    }

    toolTipSubText: {
        const parts = [];
        if (isScreenBrightnessAvailable) {
            parts.push(i18n("Scroll to adjust screen brightness"));
        }
        if (nightColorMonitor.enabled) {
            parts.push(i18n("Middle-click to toggle Night Light"));
        }
        return parts.join("\n");
    }

    Plasmoid.icon: {
        let iconName = "brightness-high";

        if (nightColorMonitor.enabled) {
            if (!nightColorMonitor.running) {
                iconName = "redshift-status-off";
            } else if (nightColorMonitor.currentTemperature != 6500) {
                if (nightColorMonitor.daylight) {
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

    onScreenBrightnessChanged: {
        if (disableBrightnessUpdate) {
            return;
        }
        const service = pmSource.serviceForSource("PowerDevil");
        const operation = service.operationDescription("setBrightness");
        operation.brightness = screenBrightness;
        // show OSD only when the plasmoid isn't expanded since the moving slider is feedback enough
        operation.silent = brightnesscontrol.expanded;
        updateScreenBrightnessJob = service.startOperationCall(operation);
        updateScreenBrightnessJob.finished.connect(job => {
            Logic.updateBrightness(brightnesscontrol, pmSource);
        });
    }

    onKeyboardBrightnessChanged: {
        if (disableBrightnessUpdate) {
            return;
        }
        var service = pmSource.serviceForSource("PowerDevil");
        var operation = service.operationDescription("setKeyboardBrightness");
        operation.brightness = keyboardBrightness;
        // show OSD only when the plasmoid isn't expanded since the moving slider is feedback enough
        operation.silent = brightnesscontrol.expanded;
        updateKeyboardBrightnessJob = service.startOperationCall(operation);
        updateKeyboardBrightnessJob.finished.connect(job => {
            Logic.updateBrightness(brightnesscontrol, pmSource);
        });
    }

    compactRepresentation: CompactRepresentation {
        isBrightnessAvailable: brightnesscontrol.isScreenBrightnessAvailable

        onWheel: wheel => {
            if (!brightnesscontrol.isScreenBrightnessAvailable) {
                return;
            }
            const delta = (wheel.inverted ? -1 : 1) * (wheel.angleDelta.y ? wheel.angleDelta.y : -wheel.angleDelta.x);

            const maximumBrightness = brightnesscontrol.maximumScreenBrightness
            // Don't allow the UI to turn off the screen
            // Please see https://git.reviewboard.kde.org/r/122505/ for more information
            const minimumBrightness = (maximumBrightness > 100 ? 1 : 0)
            const stepSize = Math.max(1, maximumBrightness / 20)

            let newBrightness;
            if (Math.abs(delta) < 120) {
                // Touchpad scrolling
                brightnessError += delta * stepSize / 120;
                const change = Math.round(brightnessError);
                brightnessError -= change;
                newBrightness = brightnesscontrol.screenBrightness + change;
            } else if (wheel.modifiers & Qt.ShiftModifier) {
                newBrightness = Math.round((Math.round(brightnesscontrol.screenBrightness * 100 / maximumBrightness) + delta/120) / 100 * maximumBrightness)
            } else {
                // Discrete/wheel scrolling
                newBrightness = Math.round(brightnesscontrol.screenBrightness/stepSize + delta/120) * stepSize;
            }
            brightnesscontrol.screenBrightness = Math.max(minimumBrightness, Math.min(maximumBrightness, newBrightness));
        }

        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        property bool wasExpanded: false
        onPressed: wasExpanded = brightnesscontrol.expanded
        onClicked: mouse => {
            if (mouse.button == Qt.MiddleButton) {
                toggleNightColorInhibition();
            } else {
                brightnesscontrol.expanded = !wasExpanded;
            }
        }

        function toggleNightColorInhibition() {
            if (!nightColorMonitor.available) {
                return;
            }
            switch (nightColorInhibitor.state) {
            case NightColorInhibitor.Inhibiting:
            case NightColorInhibitor.Inhibited:
                nightColorInhibitor.uninhibit();
                break;
            case NightColorInhibitor.Uninhibiting:
            case NightColorInhibitor.Uninhibited:
                nightColorInhibitor.inhibit();
                break;
            }
        }
    }

    fullRepresentation: PopupDialog {
        id: dialogItem

        readonly property var appletInterface: brightnesscontrol

        Layout.minimumWidth: Kirigami.Units.gridUnit * 10
        Layout.maximumWidth: Kirigami.Units.gridUnit * 80
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20

        Layout.minimumHeight: Kirigami.Units.gridUnit * 10
        Layout.maximumHeight: Kirigami.Units.gridUnit * 40
        Layout.preferredHeight: implicitHeight

        isBrightnessAvailable: brightnesscontrol.isBrightnessAvailable
        isScreenBrightnessAvailable: brightnesscontrol.isScreenBrightnessAvailable
        isKeyboardBrightnessAvailable: brightnesscontrol.isKeyboardBrightnessAvailable
    } // todo

    Component.onCompleted: {
        Logic.updateBrightness(brightnesscontrol, pmSource);
        Plasmoid.removeInternalAction("configure");
    }
}
