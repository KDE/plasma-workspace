/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021-2022 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.kcoreaddons 1.0 as KCoreAddons
import org.kde.kquickcontrolsaddons 2.1 // For KCMShell
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.plasmoid 2.0

import "logic.js" as Logic

Item {
    id: batterymonitor

    property QtObject pmSource: PlasmaCore.DataSource {
        id: pmSource
        engine: "powermanagement"
        connectedSources: sources
        onSourceAdded: {
            disconnectSource(source);
            connectSource(source);
        }
        onSourceRemoved: {
            disconnectSource(source);
        }
        onDataChanged: {
            Logic.updateBrightness(batterymonitor, pmSource);
            Logic.updateInhibitions(batterymonitor, pmSource);
        }
    }
    property QtObject batteries: PlasmaCore.SortFilterModel {
        id: batteries
        filterRole: "Is Power Supply"
        sortOrder: Qt.DescendingOrder
        sourceModel: PlasmaCore.SortFilterModel {
            sortRole: "Pretty Name"
            sortOrder: Qt.AscendingOrder
            sortCaseSensitivity: Qt.CaseInsensitive
            sourceModel: PlasmaCore.DataModel {
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
    readonly property bool kcmAuthorized: KCMShell.authorize("powerdevilprofilesconfig.desktop").length > 0
    readonly property bool kcmEnergyInformationAuthorized: KCMShell.authorize("kcm_energyinfo.desktop").length > 0
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

    function action_configure() {
        KCMShell.openSystemSettings("kcm_powerdevilprofilesconfig");
    }

    function action_energyinformationkcm() {
        KCMShell.openInfoCenter("kcm_energyinfo");
    }

    function action_showPercentage() {
        if (!Plasmoid.configuration.showPercentage) {
            Plasmoid.configuration.showPercentage = true;
        } else {
            Plasmoid.configuration.showPercentage = false;
        }
    }

    Plasmoid.switchWidth: PlasmaCore.Units.gridUnit * 10
    Plasmoid.switchHeight: PlasmaCore.Units.gridUnit * 10
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

    Plasmoid.toolTipMainText: {
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

    Plasmoid.toolTipSubText: {
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
            } else if (pmSource.data["AC Adapter"] && pmSource.data["AC Adapter"]["Plugged in"]) {
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

    Plasmoid.icon: !hasBatteries ? "video-display-brightness" : "battery"

    onScreenBrightnessChanged: {
        if (disableBrightnessUpdate) {
            return;
        }
        const service = pmSource.serviceForSource("PowerDevil");
        const operation = service.operationDescription("setBrightness");
        operation.brightness = screenBrightness;
        // show OSD only when the plasmoid isn't expanded since the moving slider is feedback enough
        operation.silent = Plasmoid.expanded;
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
        operation.silent = Plasmoid.expanded;
        updateKeyboardBrightnessJob = service.startOperationCall(operation);
        updateKeyboardBrightnessJob.finished.connect(job => {
            Logic.updateBrightness(batterymonitor, pmSource);
        });
    }

    Plasmoid.compactRepresentation: CompactRepresentation {
        hasBatteries: batterymonitor.hasBatteries
        batteries: batterymonitor.batteries
        isHeldOnPerformanceMode: batterymonitor.isHeldOnPerformanceMode
        isHeldOnPowerSaveMode: batterymonitor.isHeldOnPowerSaveMode
        isSomehowFullyCharged: batterymonitor.isSomehowFullyCharged

        onWheel: {
            const delta = wheel.angleDelta.y || wheel.angleDelta.x

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
            } else {
                // Discrete/wheel scrolling
                newBrightness = Math.round(batterymonitor.screenBrightness/stepSize + delta/120) * stepSize;
            }
            batterymonitor.screenBrightness = Math.max(minimumBrightness, Math.min(maximumBrightness, newBrightness));
        }
    }

    Plasmoid.fullRepresentation: PopupDialog {
        id: dialogItem

        readonly property var appletInterface: Plasmoid.self

        Layout.minimumWidth: PlasmaCore.Units.gridUnit * 10
        Layout.maximumWidth: PlasmaCore.Units.gridUnit * 80
        Layout.preferredWidth: PlasmaCore.Units.gridUnit * 20

        Layout.minimumHeight: PlasmaCore.Units.gridUnit * 10
        Layout.maximumHeight: PlasmaCore.Units.gridUnit * 40
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

        property int cookie1: -1
        property int cookie2: -1
        onPowerManagementChanged: disabled => {
            const service = pmSource.serviceForSource("PowerDevil");
            if (disabled) {
                const reason = i18n("The battery applet has enabled system-wide inhibition");
                const op1 = service.operationDescription("beginSuppressingSleep");
                op1.reason = reason;
                const op2 = service.operationDescription("beginSuppressingScreenPowerManagement");
                op2.reason = reason;

                const job1 = service.startOperationCall(op1);
                job1.finished.connect(job => {
                    cookie1 = job.result;
                });

                const job2 = service.startOperationCall(op2);
                job2.finished.connect(job => {
                    cookie2 = job.result;
                });
            } else {
                const op1 = service.operationDescription("stopSuppressingSleep");
                op1.cookie = cookie1;
                const op2 = service.operationDescription("stopSuppressingScreenPowerManagement");
                op2.cookie = cookie2;

                const job1 = service.startOperationCall(op1);
                job1.finished.connect(job => {
                    cookie1 = -1;
                });

                const job2 = service.startOperationCall(op2);
                job2.finished.connect(job => {
                    cookie2 = -1;
                });
            }
            batterymonitor.powermanagementDisabled = disabled
        }

        PlasmaCore.DataSource {
            id: notificationSource
            engine: "notifications"
        }

        onActivateProfileRequested: {
            dialogItem.activeProfile = profile;
            const service = pmSource.serviceForSource("PowerDevil");
            const op = service.operationDescription("setPowerProfile");
            op.profile = profile;

            const job = service.startOperationCall(op);
            job.finished.connect(job => {
                dialogItem.activeProfile = Qt.binding(() => actuallyActiveProfile);
                if (!job.result) {
                    const notifications = notificationSource.serviceForSource("notification")
                    const operation = notifications.operationDescription("createNotification");
                    operation.appName = i18n("Battery and Brightness");
                    operation.appIcon = "dialog-error";
                    operation.icon = "dialog-error";
                    operation.body = i18n("Failed to activate %1 mode", profile);
                    notifications.startOperationCall(operation);
                }
            });
        }
    }

    Component.onCompleted: {
        Logic.updateBrightness(batterymonitor, pmSource);
        Logic.updateInhibitions(batterymonitor, pmSource)

        if (batterymonitor.kcmEnergyInformationAuthorized) {
            Plasmoid.setAction("energyinformationkcm", i18n("&Show Energy Information…"), "documentinfo");
        }
        Plasmoid.setAction("showPercentage", i18n("Show Battery Percentage on Icon When Not Fully Charged"), "format-number-percent");
        Plasmoid.action("showPercentage").checkable = true;
        Plasmoid.action("showPercentage").checked = Qt.binding(() => Plasmoid.configuration.showPercentage);

        Plasmoid.removeAction("configure");
        if (batterymonitor.kcmAuthorized) {
            Plasmoid.setAction("configure", i18n("&Configure Energy Saving…"), "configure", "alt+d, s");
        }
    }
}
