/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021-2022 ivan tkachenko <me@ratijas.tk>
    SPDX-FileCopyrightText: 2023 Natalie Clarius <natalie.clarius@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.coreaddons as KCoreAddons
import org.kde.kcmutils // KCMLauncher
import org.kde.config // KAuthorized
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

import org.kde.plasma.private.batterymonitor
import org.kde.plasma.private.battery

PlasmoidItem {
    id: batterymonitor

    PowerProfilesControl {
        id: powerProfilesControl

        readonly property bool isInBalancedProfile: powerProfilesControl.activeProfile === "balanced"
        readonly property bool isInPerformanceProfile: powerProfilesControl.activeProfile === "performance"
        readonly property bool isInPowersaveProfile: powerProfilesControl.activeProfile === "power-saver"
        readonly property bool isHeldOPowerProfile: powerProfilesControl.profileHolds.length > 0
        readonly property string defaultPowerProfile: powerProfilesControl.configuredProfile ? powerProfilesControl.configuredProfile : "balanced"
        readonly property bool isInDefaultPowerProfile: powerProfilesControl.activeProfile && powerProfilesControl.activeProfile == powerProfilesControl.defaultPowerProfile
    }

    BatteryControlModel {
        id: batteryControl

        readonly property int remainingTime: batteryControl.smoothedRemainingMsec
        readonly property bool isSomehowFullyCharged: batteryControl.pluggedIn && batteryControl.state === BatteryControlModel.FullyCharged
    }

    PowerManagementControl {
        id: powerManagementControl
    }

    readonly property bool kcmAuthorized: KAuthorized.authorizeControlModule("powerdevilprofilesconfig")
    readonly property bool kcmEnergyInformationAuthorized: KAuthorized.authorizeControlModule("kcm_energyinfo")

    readonly property bool inPanel: (Plasmoid.location === PlasmaCore.Types.TopEdge
        || Plasmoid.location === PlasmaCore.Types.RightEdge
        || Plasmoid.location === PlasmaCore.Types.BottomEdge
        || Plasmoid.location === PlasmaCore.Types.LeftEdge)


    function symbolicizeIconName(iconName) {
        const symbolicSuffix = "-symbolic";
        if (iconName.endsWith(symbolicSuffix)) {
            return iconName;
        }

        return iconName + symbolicSuffix;
    }

    signal inhibitionChangeRequested(bool inhibit)

    onInhibitionChangeRequested: inhibit => {

        powerManagementControl.isSilent = batterymonitor.expanded; // show OSD only when the plasmoid isn't expanded since the changing switch is feedback enough

        if (inhibit) {
            const reason = i18n("The battery applet has enabled suppressing sleep and screen locking");
            powerManagementControl.inhibit(reason)
        } else {
            powerManagementControl.uninhibit()
        }
    }

    signal activateProfileRequested(string profile)

    onActivateProfileRequested: profile => {
        powerProfilesControl.isSilent = batterymonitor.expanded;

        if (profile === powerProfilesControl.activeProfile) {
            return;
        }
        powerProfilesControl.setProfile(profile);
    }


    switchWidth: Kirigami.Units.gridUnit * 10
    switchHeight: Kirigami.Units.gridUnit * 10

    Plasmoid.title: batteryControl.hasBatteries ? i18n("Power and Battery") : i18n("Power Management")

    LayoutMirroring.enabled: Qt.application.layoutDirection == Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Plasmoid.status: {

        if (powerManagementControl.isManuallyInhibited || !powerProfilesControl.isInDefaultPowerProfile) {
            return PlasmaCore.Types.ActiveStatus;
        }

        if (batteryControl.hasCumulative && batteryControl.state === BatteryControlModel.Discharging) {
            return PlasmaCore.Types.ActiveStatus;
        }

        return PlasmaCore.Types.PassiveStatus;
    }

    toolTipMainText: {
        if(batteryControl.hasInternalBatteries && !batteryControl.hasCumulative){
            return i18n("Battery is not present in the bay");
        }

        if (!batteryControl.hasInternalBatteries) {
            return Plasmoid.title
        } else if (batteryControl.isSomehowFullyCharged) {
            return i18n("Fully Charged");
        }

        const percent = batteryControl.percent;
        if (batteryControl.pluggedIn) {
            const state = batteryControl.state;
            if (state === BatteryControlModel.NoCharge) {
                return i18n("Battery at %1%, not Charging", percent);
            } else if (state === BatteryControlModel.Discharging) {
                return i18n("Battery at %1%, plugged in but still discharging", percent);
            } else if (state === BatteryControlModel.Charging) {
                return i18n("Battery at %1%, Charging", percent);
            }
        }
        return i18n("Battery at %1%", percent);
    }

    toolTipSubText: {
        const parts = [];

        // Add special text for the "plugged in but still discharging" case
        if (batteryControl.pluggedIn && batteryControl.state === BatteryControlModel.Discharging) {
            parts.push(i18n("The power supply is not powerful enough to charge the battery"));
        }

        if (!batteryControl.hasBatteries) {
            parts.push(i18n("No Batteries Available"));
        } else if(batteryControl.hasInternalBatteries) {
            if (batteryControl.remainingTime > 0) {
                const remainingTimeString = KCoreAddons.Format.formatDuration(batteryControl.remainingTime, KCoreAddons.FormatTypes.HideSeconds);
                if (batteryControl.state === BatteryControlModel.FullyCharged) {
                    // Don't add anything
                } else if (batteryControl.pluggedIn && batteryControl.state === BatteryControlModel.Charging) {
                    parts.push(i18nc("time until fully charged - HH:MM", "%1 until fully charged", remainingTimeString));
                } else {
                    parts.push(i18nc("remaining time left of battery usage - HH:MM", "%1 remaining", remainingTimeString));
                }
            } else if (batteryControl.state === BatteryControlModel.NoCharge && !batteryControl.isSomehowFullyCharged) {
                parts.push(i18n("Not charging"));
            } // otherwise, don't add anything
        }

        if (powerManagementControl.isManuallyInhibited) {
            parts.push(i18n("Automatic sleep and screen locking are disabled; middle-click to re-enable"));
        } else {
            parts.push(i18n("Middle-click to disable automatic sleep and screen locking"));
        }

        if (powerProfilesControl.isInPerformanceProfile) {
            if (powerProfilesControl.isHeldOnPowerProfile) {
                parts.push(i18np("An application has requested activating Performance mode",
                                 "%1 applications have requested activating Performance mode",
                                 powerProfilesControl.activeProfileHolds.length));
            } else {
                parts.push(i18n("System is in Performance mode; scroll to change"));
            }
        } else if (powerProfilesControl.isInPowersaveProfile) {
            if (powerProfilesControl.isHeldOnPowerProfile) {
                parts.push(i18np("An application has requested activating Power Save mode",
                                "%1 applications have requested activating Power Save mode",
                                powerProfilesControl.activeProfileHolds.length));
            } else {
                parts.push(i18n("System is in Power Save mode; scroll to change"));
            }
        } else if (powerProfilesControl.isInBalancedProfile) {
            parts.push(i18n("System is in Balanced Power mode; scroll to change"));
        }

        return parts.join("\n");
    }

    Plasmoid.icon: {
        let iconName;
        if (batteryControl.hasBatteries) {
            iconName = "battery-full";
        } else {
            iconName = "speedometer";
        }

        if (inPanel) {
            return symbolicizeIconName(iconName);
        }

        return iconName;
    }

    compactRepresentation: CompactRepresentation {
        batteryPercent: batteryControl.percent
        batteryPluggedIn: batteryControl.pluggedIn
        hasBatteries: batteryControl.hasBatteries
        hasInternalBatteries: batteryControl.hasInternalBatteries
        hasCumulative: batteryControl.hasCumulative

        isSomehowFullyCharged: batteryControl.isSomehowFullyCharged
        isDischarging: !batteryControl.pluggedIn

        isManuallyInhibited: powerManagementControl.isManuallyInhibited
        activeProfile: powerProfilesControl.activeProfile
        isInDefaultPowerProfile: powerProfilesControl.isInDefaultPowerProfile

        model: batteryControl

        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        property bool wasExpanded: false
        onPressed: wasExpanded = batterymonitor.expanded
        onClicked: mouse => {
            if (mouse.button == Qt.MiddleButton) {
                batterymonitor.inhibitionChangeRequested(!powerManagementControl.isManuallyInhibited);
            } else {
                batterymonitor.expanded = !wasExpanded;
            }
        }

        onWheel: wheel => {

            if(!powerProfilesControl.isPowerProfileDaemonInstalled){
                return;
            }

            let profiles = powerProfilesControl.profiles;
            if (!profiles.length) {
                return;
            }

            let activeProfile = powerProfilesControl.activeProfile;
            let newProfile = activeProfile;

            const delta = (wheel.inverted ? -1 : 1) * (wheel.angleDelta.y ? wheel.angleDelta.y : -wheel.angleDelta.x);
            // Magic number 120 for common "one click"
            // See: https://qt-project.org/doc/qt-5/qml-qtquick-wheelevent.html#angleDelta-prop
            const steps = Math.round(delta/120);
            const newProfileIndex = Math.max(0, Math.min(profiles.length - 1, profiles.indexOf(activeProfile) + steps));
            batterymonitor.activateProfileRequested(profiles[newProfileIndex])
        }
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

        model: batteryControl

        isManuallyInhibited: powerManagementControl.isManuallyInhibited
        isManuallyInhibitedError: powerManagementControl.isManuallyInhibitedError
        pluggedIn: batteryControl.pluggedIn
        chargeStopThreshold: batteryControl.chargeStopThreshold
        remainingTime: batteryControl.remainingTime
        activeProfile: powerProfilesControl.activeProfile
        activeProfileError: powerProfilesControl.profileError
        inhibitions: powerManagementControl.inhibitions
        inhibitsLidAction: powerManagementControl.isLidPresent && !powerManagementControl.triggersLidAction
        profilesInstalled: powerProfilesControl.isPowerProfileDaemonInstalled
        profiles: powerProfilesControl.profiles
        inhibitionReason: powerProfilesControl.inhibitionReason
        degradationReason: powerProfilesControl.degradationReason
        profileHolds: powerProfilesControl.profileHolds

        onActivateProfileRequested: profile => {
            batterymonitor.activateProfileRequested(profile);
        }

        onInhibitionChangeRequested: inhibit => {
            batterymonitor.inhibitionChangeRequested(inhibit);
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
            visible: batteryControl.hasBatteries
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
