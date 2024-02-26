/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021-2022 ivan tkachenko <me@ratijas.tk>

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

import org.kde.plasma.private.battery

import "logic.js" as Logic

PlasmoidItem {
    id: batterymonitor

    PowerProfilesControl {
        id: powerProfilesControl
    }

    BatteryControl {
        id: batteryControl
    }

    PowerManagmentControl {
        id: powerManagmentControl
    }

    readonly property bool hasBatteries: batteryControl.batteries.count > 0 && batteryControl.hasCumulative
    readonly property bool kcmAuthorized: KAuthorized.authorizeControlModule("powerdevilprofilesconfig")
    readonly property bool kcmEnergyInformationAuthorized: KAuthorized.authorizeControlModule("kcm_energyinfo")
    readonly property bool isSomehowFullyCharged: (batteryControl.pluggedIn && batteryControl.state === "FullyCharged") ||
                                                   // When we are using a charge threshold, the kernel
                                                   // may stop charging within a percentage point of the actual threshold
                                                   // and this is considered correct behavior, so we have to handle
                                                   // that. See https://bugzilla.kernel.org/show_bug.cgi?id=215531.
                                                   (batteryControl.pluggedIn && (batteryControl.percent  >= batteryControl.chargeStopThreshold - 1
                                                       && batteryControl.percent  <= batteryControl.chargeStopThreshold + 1)
                                                   // Also, Upower may give us a status of "Not charging" rather than
                                                   // "Fully charged", so we need to account for that as well. See
                                                   // https://gitlab.freedesktop.org/upower/upower/-/issues/142.
                                                   && (batteryControl.state === "NoCharge" || batteryControl.state === "FullyCharged"))
    readonly property int remainingTime: Number(batteryControl.smoothedRemainingMsec)

    property bool isManuallyInPerformanceMode: false // to be set on power profile requested through the applet
    property bool isManuallyInPowerSaveMode: false // to be set on power profile requested through the applet
    readonly property bool isSomehowInPerformanceMode: powerProfilesControl.actuallyActiveProfile === "performance"// Don't care about whether it was manually one or due to holds
    readonly property bool isSomehowInPowerSaveMode: powerProfilesControl.actuallyActiveProfile === "power-saver" // Don't care about whether it was manually one or due to holds
    readonly property bool isHeldOnPerformanceMode: isSomehowInPerformanceMode && powerProfilesControl.activeProfileHolds.length > 0
    readonly property bool isHeldOnPowerSaveMode: isSomehowInPowerSaveMode && powerProfilesControl.activeProfileHolds.length > 0

    readonly property bool inPanel: (Plasmoid.location === PlasmaCore.Types.TopEdge
        || Plasmoid.location === PlasmaCore.Types.RightEdge
        || Plasmoid.location === PlasmaCore.Types.BottomEdge
        || Plasmoid.location === PlasmaCore.Types.LeftEdge)

    // List of active power management inhibitions (applications that are
    // blocking sleep and screen locking).
    //
    // type: [{
    //  Name: string,
    //  PrettyName: string
    //  Icon: string,
    //  Reason: string,
    // }]
    property var inhibitions: powerManagmentControl.inhibitions

    function symbolicizeIconName(iconName) {
        const symbolicSuffix = "-symbolic";
        if (iconName.endsWith(symbolicSuffix)) {
            return iconName;
        }

        return iconName + symbolicSuffix;
    }

    switchWidth: Kirigami.Units.gridUnit * 10
    switchHeight: Kirigami.Units.gridUnit * 10

    Plasmoid.title: hasBatteries ? i18n("Power and Battery") : i18n("Power Management")

    LayoutMirroring.enabled: Qt.application.layoutDirection == Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Plasmoid.status: {

        if (batteryControl.hasCumulative && batteryControl.state === "Discharging") {
            return PlasmaCore.Types.ActiveStatus;
        }

        if (isManuallyInPerformanceMode || isManuallyInPowerSaveMode || isHeldOnPerformanceMode || isHeldOnPowerSaveMode) {
            return PlasmaCore.Types.ActiveStatus;
        }

        return PlasmaCore.Types.PassiveStatus;
    }

    toolTipMainText: {
        if (!hasBatteries) {
            return Plasmoid.title
        } else if (isSomehowFullyCharged) {
            return i18n("Fully Charged");
        }

        const percent = batteryControl.percent;
        if (batteryControl.pluggedIn) {
            const state = batteryControl.state;
            if (state === "NoCharge" && typeof state !== undefined) {
                return i18n("Battery at %1%, not Charging", percent);
            } else if (state === "Discharging") {
                return i18n("Battery at %1%, plugged in but still discharging", percent);
            } else if (state === "Charging") {
                return i18n("Battery at %1%, Charging", percent);
            }
        }
        return i18n("Battery at %1%", percent);
    }

    toolTipSubText: {
        const parts = [];

        // Add special text for the "plugged in but still discharging" case
        if (batteryControl.pluggedIn && batteryControl.state === "Discharging") {
            parts.push(i18n("The power supply is not powerful enough to charge the battery"));
        }

        if (batteryControl.batteries.count === 0) {
            parts.push(i18n("No Batteries Available"));
        } else if (remainingTime > 0) {
            const remainingTimeString = KCoreAddons.Format.formatDuration(remainingTime, KCoreAddons.FormatTypes.HideSeconds);
            if (batteryControl.state === "FullyCharged") {
                // Don't add anything
            } else if (batteryControl.pluggedIn && batteryControl.state === "Charging") {
                parts.push(i18nc("time until fully charged - HH:MM","%1 until fully charged", remainingTimeString));
            } else {
                parts.push(i18nc("remaining time left of battery usage - HH:MM","%1 remaining", remainingTimeString));
            }
        } else if (batteryControl.state === "NoCharge" && !isSomehowFullyCharged) {
            parts.push(i18n("Not charging"));
        } // otherwise, don't add anything

        if (isSomehowInPerformanceMode) {
            if (isHeldOnPerformanceMode) {
                parts.push(i18np("An application has requested activating Performance mode",
                                 "%1 applications have requested activating Performance mode",
                                 powerProfilesControl.activeProfileHolds.length));
            } else {
                parts.push(i18n("System is in Performance mode"));
            }
        } else if (isSomehowInPowerSaveMode) {
            if (isHeldOnPowerSaveMode) {
                parts.push(i18np("An application has requested activating Power Save mode",
                                "%1 applications have requested activating Power Save mode",
                                powerProfilesControl.activeProfileHolds.length));
            } else {
                parts.push(i18n("System is in Power Save mode"));
            }
        }

        return parts.join("\n");
    }

    Plasmoid.icon: {
        let iconName;
        if (hasBatteries) {
            iconName = "battery-full";
        } else {
            iconName = "battery-profile-performance";
        }

        if (inPanel) {
            return symbolicizeIconName(iconName);
        }

        return iconName;
    }

    compactRepresentation: CompactRepresentation {
        batteryPercent: batteryControl.percent
        batteryPluggedIn: batteryControl.pluggedIn
        hasBatteries: batterymonitor.hasBatteries
        batteries: batteryControl.batteries
        isSetToPerformanceMode: batterymonitor.isHeldOnPerformanceMode || batterymonitor.isManuallyInPerformanceMode
        isSetToPowerSaveMode: batterymonitor.isHeldOnPowerSaveMode || batterymonitor.isManuallyInPowerSaveMode
        isSomehowFullyCharged: batterymonitor.isSomehowFullyCharged
    }

    fullRepresentation: PopupDialog {
        id: dialogItem

        readonly property var appletInterface: batterymonitor

        Layout.minimumWidth: Kirigami.Units.gridUnit * 10
        Layout.maximumWidth: Kirigami.Units.gridUnit * 80
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20

        Layout.minimumHeight: Kirigami.Units.gridUnit * 10
        Layout.maximumHeight: Kirigami.Units.gridUnit * 40
        Layout.preferredHeight: implicitHeight

        model: batteryControl.batteries

        pluggedIn: batteryControl.pluggedIn
        chargeStopThreshold: batteryControl.chargeStopThreshold
        percent: batteryControl.percent
        state: batteryControl.state
        remainingTime: batterymonitor.remainingTime
        activeProfile: powerProfilesControl.actuallyActiveProfile
        inhibitions: batterymonitor.inhibitions
        inhibitsLidAction: powerManagmentControl.isLidPresent && powerManagmentControl.triggersLidAction
        profilesInstalled: powerProfilesControl.isPowerProfileDaemonInstalled
        profiles: powerProfilesControl.profiles
        inhibitionReason: powerProfilesControl.inhibitionReason
        degradationReason: powerProfilesControl.degradationReason
        profileHolds: powerProfilesControl.activeProfileHolds

        onInhibitionChangeRequested: inhibit => {
            if (inhibit) {
                const sleepSuppressingReason = i18n("The battery applet has enabled suppressing sleep");
                powerManagmentControl.beginSuppressingSleep(sleepSuppressingReason);
                const screenPowerManagmentReason = i18n("The battery applet has enabled suppressing screen power managment")
                powerManagmentControl.beginSuppressingScreenPowerManagement(screenPowerManagmentReason);
            } else {
                powerManagmentControl.stopSuppressingSleep();
                powerManagmentControl.stopSuppressingScreenPowerManagement();
            }
        }

        Notification {
            id: powerProfileError
            componentName: "plasma_workspace"
            eventId: "warning"
            iconName: "speedometer"
            title: i18n("Power Management")
        }

        onActivateProfileRequested: profile => {
            if(powerProfilesControl.setActuallyActiveProfile(profile)) {
                batterymonitor.isManuallyInPerformanceMode = profile == "performance";
                batterymonitor.isManuallyInPowerSaveMode = profile == "power-saver";
            } else {
                powerProfileError.text = i18n("Failed to activate %1 mode", profile);
                powerProfileError.sendEvent();
            }
        }
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: i18n("&Show Energy Information…")
            icon.name: "documentinfo"
            visible: batterymonitor.kcmEnergyInformationAuthorized
            onTriggered: KCMLauncher.openInfoCenter("kcm_energyinfo")
        },
        PlasmaCore.Action {
            text: i18n("Show Battery Percentage on Icon When Not Fully Charged")
            icon.name: "format-number-percent"
            visible: batterymonitor.hasBatteries
            checkable: true
            checked: Plasmoid.configuration.showPercentage
            onTriggered: checked => {
                Plasmoid.configuration.showPercentage = checked
            }
        }
    ]

    PlasmaCore.Action {
        id: configureAction
        text: i18n("&Configure Power Management…")
        icon.name: "configure"
        shortcut: "alt+d, s"
        onTriggered: {
            KCMLauncher.openSystemSettings("kcm_powerdevilprofilesconfig");
        }
    }

    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction);
    }
}
