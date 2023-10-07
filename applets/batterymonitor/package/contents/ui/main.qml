/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021-2022 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.coreaddons 1.0 as KCoreAddons
import org.kde.kcmutils // KCMLauncher
import org.kde.config // KAuthorized
import org.kde.notification 1.0
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support 2.0 as P5Support
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels

import "logic.js" as Logic

PlasmoidItem {
    id: batterymonitor

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
            Logic.updateBrightness(batterymonitor, pmSource);
            Logic.updateInhibitions(batterymonitor, pmSource);
        }
    }
    property QtObject batteries: KItemModels.KSortFilterProxyModel {
        id: batteries
        filterRoleName: "Is Power Supply"
        sortOrder: Qt.DescendingOrder
        sourceModel: KItemModels.KSortFilterProxyModel {
            sortRoleName: "Pretty Name"
            sortOrder: Qt.AscendingOrder
            sortCaseSensitivity: Qt.CaseInsensitive
            sourceModel: P5Support.DataModel {
                dataSource: pmSource
                sourceFilter: "Battery[0-9]+"
            }
        }
    }
    property QtObject updateScreenBrightnessJob
    property QtObject updateKeyboardBrightnessJob

    readonly property bool isBrightnessAvailable: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Screen Brightness Available"] ? true : false
    readonly property bool isKeyboardBrightnessAvailable: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Keyboard Brightness Available"] ? true : false
    readonly property bool hasBatteries: batteries.count > 0 && pmSource.data["Battery"]["Has Cumulative"]
    readonly property bool hasBrightness: isBrightnessAvailable || isKeyboardBrightnessAvailable
    readonly property bool kcmAuthorized: KAuthorized.authorizeControlModule("powerdevilprofilesconfig")
    readonly property bool kcmEnergyInformationAuthorized: KAuthorized.authorizeControlModule("kcm_energyinfo")
    readonly property bool isSomehowInPerformanceMode: actuallyActiveProfile === "performance"// Don't care about whether it was manually one or due to holds
    readonly property bool isSomehowinPowerSaveMode: actuallyActiveProfile === "power-saver" // Don't care about whether it was manually one or due to holds
    readonly property bool isHeldOnPerformanceMode: isSomehowInPerformanceMode && activeProfileHolds.length > 0
    readonly property bool isHeldOnPowerSaveMode: isSomehowinPowerSaveMode && activeProfileHolds.length > 0
    readonly property int maximumScreenBrightness: pmSource.data["PowerDevil"] ? pmSource.data["PowerDevil"]["Maximum Screen Brightness"] || 0 : 0
    readonly property int maximumKeyboardBrightness: pmSource.data["PowerDevil"] ? pmSource.data["PowerDevil"]["Maximum Keyboard Brightness"] || 0 : 0
    readonly property bool isSomehowFullyCharged: (pmSource.data["AC Adapter"]["Plugged in"] && pmSource.data["Battery"]["State"] === "FullyCharged") ||
                                                   // When we are using a charge threshold, the kernel
                                                   // may stop charging within a percentage point of the actual threshold
                                                   // and this is considered correct behavior, so we have to handle
                                                   // that. See https://bugzilla.kernel.org/show_bug.cgi?id=215531.
                                                   (pmSource.data["AC Adapter"]["Plugged in"]
                                                   && typeof pmSource.data["Battery"]["Charge Stop Threshold"] === "number"
                                                   && (pmSource.data.Battery.Percent  >= pmSource.data["Battery"]["Charge Stop Threshold"] - 1
                                                       && pmSource.data.Battery.Percent  <= pmSource.data["Battery"]["Charge Stop Threshold"] + 1)
                                                   // Also, Upower may give us a status of "Not charging" rather than
                                                   // "Fully charged", so we need to account for that as well. See
                                                   // https://gitlab.freedesktop.org/upower/upower/-/issues/142.
                                                   && (pmSource.data["Battery"]["State"] === "NoCharge" || pmSource.data["Battery"]["State"] === "FullyCharged"))
    readonly property int remainingTime: Number(pmSource.data["Battery"]["Smoothed Remaining msec"])

    readonly property bool inPanel: (Plasmoid.location === PlasmaCore.Types.TopEdge
        || Plasmoid.location === PlasmaCore.Types.RightEdge
        || Plasmoid.location === PlasmaCore.Types.BottomEdge
        || Plasmoid.location === PlasmaCore.Types.LeftEdge)

    property bool powermanagementDisabled: false
    property bool disableBrightnessUpdate: true
    property int screenBrightness
    property int keyboardBrightness

    // List of active power management inhibitions (applications that are
    // blocking sleep and screen locking).
    //
    // type: [{
    //  Icon: string,
    //  Name: string,
    //  Reason: string,
    // }]
    property var inhibitions: []
    readonly property var activeProfileHolds: pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Profile Holds"] || []) : []
    readonly property string actuallyActiveProfile: pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Current Profile"] || "") : ""

    function symbolicizeIconName(iconName) {
        const symbolicSuffix = "-symbolic";
        if (iconName.endsWith(symbolicSuffix)) {
            return iconName;
        }

        return iconName + symbolicSuffix;
    }

    switchWidth: Kirigami.Units.gridUnit * 10
    switchHeight: Kirigami.Units.gridUnit * 10

    Plasmoid.title: (hasBatteries && hasBrightness ? i18n("Battery and Brightness") :
                                     hasBrightness ? i18n("Brightness") :
                                     hasBatteries ? i18n("Battery") : i18n("Power Management"))

    LayoutMirroring.enabled: Qt.application.layoutDirection == Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Plasmoid.status: {
        if (powermanagementDisabled) {
            return PlasmaCore.Types.ActiveStatus;
        }

        if (pmSource.data.Battery["Has Cumulative"] && !isSomehowFullyCharged) {
            return PlasmaCore.Types.ActiveStatus;
        }

        if (isHeldOnPerformanceMode || isHeldOnPowerSaveMode) {
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

        const percent = pmSource.data.Battery.Percent;
        if (pmSource.data["AC Adapter"] && pmSource.data["AC Adapter"]["Plugged in"]) {
            const state = pmSource.data.Battery.State;
            if (state === "NoCharge") {
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
        if (pmSource.data["AC Adapter"] && pmSource.data["AC Adapter"]["Plugged in"] && pmSource.data.Battery.State === "Discharging") {
            parts.push(i18n("The power supply is not powerful enough to charge the battery"));
        }

        if (batteries.count === 0) {
            parts.push(i18n("No Batteries Available"));
        } else if (remainingTime > 0) {
            const remainingTimeString = KCoreAddons.Format.formatDuration(remainingTime, KCoreAddons.FormatTypes.HideSeconds);
            if (pmSource.data["Battery"]["State"] === "FullyCharged") {
                // Don't add anything
            } else if (pmSource.data["AC Adapter"] && pmSource.data["AC Adapter"]["Plugged in"] && pmSource.data.Battery.State === "Charging") {
                parts.push(i18nc("time until fully charged - HH:MM","%1 until fully charged", remainingTimeString));
            } else {
                parts.push(i18nc("remaining time left of battery usage - HH:MM","%1 remaining", remainingTimeString));
            }
        } else if (pmSource.data.Battery.State === "NoCharge" && !isSomehowFullyCharged) {
            parts.push(i18n("Not charging"));
        } // otherwise, don't add anything

        if (powermanagementDisabled) {
            parts.push(i18n("Automatic sleep and screen locking are disabled"));
        }

        if (isSomehowInPerformanceMode) {
            if (isHeldOnPerformanceMode) {
                parts.push(i18np("An application has requested activating Performance mode",
                                 "%1 applications have requested activating Performance mode",
                                 activeProfileHolds.length));
            } else {
                parts.push(i18n("System is in Performance mode"));
            }
        } else if (isSomehowinPowerSaveMode) {
            if (isHeldOnPowerSaveMode) {
                parts.push(i18np("An application has requested activating Power Save mode",
                                "%1 applications have requested activating Power Save mode",
                                activeProfileHolds.length));
            } else {
                parts.push(i18n("System is in Power Save mode"));
            }
        }

        if (isBrightnessAvailable) {
            parts.push(i18n("Scroll to adjust screen brightness"));
        }

        return parts.join("\n");
    }

    Plasmoid.icon: {
        let iconName;
        if (hasBatteries) {
            iconName = "battery-full";
        } else {
            iconName = "video-display-brightness";
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
        operation.silent = batterymonitor.expanded;
        updateScreenBrightnessJob = service.startOperationCall(operation);
        updateScreenBrightnessJob.finished.connect(job => {
            Logic.updateBrightness(batterymonitor, pmSource);
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
        operation.silent = batterymonitor.expanded;
        updateKeyboardBrightnessJob = service.startOperationCall(operation);
        updateKeyboardBrightnessJob.finished.connect(job => {
            Logic.updateBrightness(batterymonitor, pmSource);
        });
    }

    compactRepresentation: CompactRepresentation {
        hasBatteries: batterymonitor.hasBatteries
        batteries: batterymonitor.batteries
        isHeldOnPerformanceMode: batterymonitor.isHeldOnPerformanceMode
        isHeldOnPowerSaveMode: batterymonitor.isHeldOnPowerSaveMode
        isSomehowFullyCharged: batterymonitor.isSomehowFullyCharged

        onWheel: wheel => {
            const delta = (wheel.inverted ? -1 : 1) * (wheel.angleDelta.y ? wheel.angleDelta.y : -wheel.angleDelta.x);

            const maximumBrightness = batterymonitor.maximumScreenBrightness
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
                newBrightness = batterymonitor.screenBrightness + change;
            } else if (wheel.modifiers & Qt.ShiftModifier) {
                newBrightness = Math.round((Math.round(batterymonitor.screenBrightness * 100 / maximumBrightness) + delta/120) / 100 * maximumBrightness)
            } else {
                // Discrete/wheel scrolling
                newBrightness = Math.round(batterymonitor.screenBrightness/stepSize + delta/120) * stepSize;
            }
            batterymonitor.screenBrightness = Math.max(minimumBrightness, Math.min(maximumBrightness, newBrightness));
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

        model: batteries

        isBrightnessAvailable: batterymonitor.isBrightnessAvailable
        isKeyboardBrightnessAvailable: batterymonitor.isKeyboardBrightnessAvailable

        pluggedIn: pmSource.data["AC Adapter"] !== undefined && pmSource.data["AC Adapter"]["Plugged in"]
        remainingTime: batterymonitor.remainingTime
        activeProfile: batterymonitor.actuallyActiveProfile
        inhibitions: batterymonitor.inhibitions
        inhibitsLidAction: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Is Lid Present"] && !pmSource.data["PowerDevil"]["Triggers Lid Action"] ? true : false
        profiles: pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Profiles"] || []) : []
        inhibitionReason: pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Performance Inhibited Reason"] || "") : ""
        degradationReason: pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Performance Degraded Reason"] || "") : ""
        profileHolds: batterymonitor.activeProfileHolds

        onInhibitionChangeRequested: inhibit => {
            const service = pmSource.serviceForSource("PowerDevil");
            if (inhibit) {
                const reason = i18n("The battery applet has enabled system-wide inhibition");
                const op1 = service.operationDescription("beginSuppressingSleep");
                op1.reason = reason;
                const op2 = service.operationDescription("beginSuppressingScreenPowerManagement");
                op2.reason = reason;

                const job1 = service.startOperationCall(op1);
                const job2 = service.startOperationCall(op2);
            } else {
                const op1 = service.operationDescription("stopSuppressingSleep");
                const op2 = service.operationDescription("stopSuppressingScreenPowerManagement");

                const job1 = service.startOperationCall(op1);
                const job2 = service.startOperationCall(op2);
            }
        }
        onPowerManagementChanged: disabled => {
            batterymonitor.powermanagementDisabled = disabled
        }

        Notification {
            id: powerProfileError
            componentName: "plasma_workspace"
            eventId: "warning"
            iconName: "speedometer"
            title: i18n("Battery and Brightness")
        }

        onActivateProfileRequested: profile => {
            dialogItem.activeProfile = profile;
            const service = pmSource.serviceForSource("PowerDevil");
            const op = service.operationDescription("setPowerProfile");
            op.profile = profile;

            const job = service.startOperationCall(op);
            job.finished.connect(job => {
                dialogItem.activeProfile = Qt.binding(() => actuallyActiveProfile);
                if (!job.result) {
                    powerProfileError.text = i18n("Failed to activate %1 mode", profile);
                    powerProfileError.sendEvent();
                }
            });
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
            checkable: true
            checked: Plasmoid.configuration.showPercentage
            onTriggered: checked => {
                Plasmoid.configuration.showPercentage = checked
            }
        }
    ]

    PlasmaCore.Action {
        id: configureAction
        text: i18n("&Configure Energy Saving…")
        icon.name: "configure"
        shortcut: "alt+d, s"
        onTriggered: {
            KCMLauncher.openSystemSettings("kcm_powerdevilprofilesconfig");
        }
    }

    Component.onCompleted: {
        Logic.updateBrightness(batterymonitor, pmSource);
        Logic.updateInhibitions(batterymonitor, pmSource)

        Plasmoid.setInternalAction("configure", configureAction);
    }
}
