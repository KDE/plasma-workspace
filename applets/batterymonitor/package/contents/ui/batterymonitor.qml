/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kcoreaddons 1.0 as KCoreAddons
import org.kde.kquickcontrolsaddons 2.0 // For KCMShell
import "logic.js" as Logic

Item {
    id: batterymonitor
    Plasmoid.switchWidth: PlasmaCore.Units.gridUnit * 10
    Plasmoid.switchHeight: PlasmaCore.Units.gridUnit * 10

    LayoutMirroring.enabled: Qt.application.layoutDirection == Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Plasmoid.status: {
        if (powermanagementDisabled) {
            return PlasmaCore.Types.ActiveStatus
        }

        if (pmSource.data.Battery["Has Cumulative"]
            && !(pmSource.data["Battery"]["State"] === "FullyCharged" && pmSource.data["AC Adapter"]["Plugged in"])) {
            return PlasmaCore.Types.ActiveStatus
        }

        return PlasmaCore.Types.PassiveStatus
    }

    Plasmoid.toolTipMainText: {
        if (batteries.count === 0 || !pmSource.data["Battery"]["Has Cumulative"]) {
            return plasmoid.title;
        }

        const percent = pmSource.data.Battery.Percent;
        const state = pmSource.data.Battery.State;

        if (state === "FullyCharged") {
            return i18n("Fully Charged");
        }

        return i18n("Battery at %1%", percent);
    }

    Plasmoid.toolTipSubText: {
        const parts = [];
        const state = pmSource.data.Battery.State;

        // === header part ===

        if (batteries.count === 0) {
            parts.push("No Batteries Available");
        } else if (remainingTime > 0) {
            const remainingTimeString = KCoreAddons.Format.formatDuration(remainingTime, KCoreAddons.FormatTypes.HideSeconds);
            // Don't add anything for fully charged
            if (state !== "FullyCharged") {
                if (isPluggedInToACAdapter) {
                    parts.push(i18nc("Time until fully charged - HH:MM", "%1 until fully charged", remainingTimeString));
                } else {
                    parts.push(i18nc("Remaining time left of battery usage - HH:MM", "%1 remaining", remainingTimeString));
                }
            }
        } else if (state === "NoCharge") {
            parts.push(i18n("Not Charging"));
        } // Otherwise, don't add anything

        // === auxiliary parts ===

        // Add special text for the "plugged in but still discharging" case
        if (isPluggedInToACAdapter && state === "Discharging") {
            if (parts.length !== 0) {
                parts.push(""); // add separator if needed
            }
            parts.push(i18n("Plugged in but still discharging."));
            parts.push(i18nc("Insert a line break if needed to match the width of messages above",
                             "The power supply is not powerful\nenough to charge the battery."));
        }

        if (powermanagementDisabled || inhibitions.length !== 0) {
            if (parts.length !== 0) {
                parts.push(""); // add separator if needed
            }
            parts.push(i18n("Automatic sleep and screen locking are disabled."));
        }
        return parts.join("\n");
    }

    Plasmoid.icon: "battery"

    property bool disableBrightnessUpdate: true

    property int screenBrightness
    readonly property int maximumScreenBrightness: pmSource.data["PowerDevil"] ? pmSource.data["PowerDevil"]["Maximum Screen Brightness"] || 0 : 0

    property int keyboardBrightness
    readonly property int maximumKeyboardBrightness: pmSource.data["PowerDevil"] ? pmSource.data["PowerDevil"]["Maximum Keyboard Brightness"] || 0 : 0

    readonly property bool isPluggedInToACAdapter: pmSource.data["AC Adapter"] ? pmSource.data["AC Adapter"]["Plugged in"] : false

    readonly property int remainingTime: Number(pmSource.data["Battery"]["Remaining msec"])

    property bool powermanagementDisabled: false

    property var inhibitions: []

    readonly property bool kcmAuthorized: KCMShell.authorize("powerdevilprofilesconfig.desktop").length > 0

    readonly property bool kcmEnergyInformationAuthorized: KCMShell.authorize("kcm_energyinfo.desktop").length > 0

    property QtObject updateScreenBrightnessJob
    onScreenBrightnessChanged: {
        if (disableBrightnessUpdate) {
            return;
        }
        var service = pmSource.serviceForSource("PowerDevil");
        var operation = service.operationDescription("setBrightness");
        operation.brightness = screenBrightness;
        // show OSD only when the plasmoid isn't expanded since the moving slider is feedback enough
        operation.silent = plasmoid.expanded
        updateScreenBrightnessJob = service.startOperationCall(operation);
        updateScreenBrightnessJob.finished.connect(function(job) {
            Logic.updateBrightness(batterymonitor, pmSource);
        });
    }

    property QtObject updateKeyboardBrightnessJob
    onKeyboardBrightnessChanged: {
        if (disableBrightnessUpdate) {
            return;
        }
        var service = pmSource.serviceForSource("PowerDevil");
        var operation = service.operationDescription("setKeyboardBrightness");
        operation.brightness = keyboardBrightness;
        operation.silent = plasmoid.expanded
        updateKeyboardBrightnessJob = service.startOperationCall(operation);
        updateKeyboardBrightnessJob.finished.connect(function(job) {
            Logic.updateBrightness(batterymonitor, pmSource);
        });
    }

    function action_configure() {
        KCMShell.openSystemSettings("kcm_powerdevilprofilesconfig");
    }

    function action_energyinformationkcm() {
        KCMShell.openInfoCenter("kcm_energyinfo");
    }

    function action_showPercentage() {
        if (!plasmoid.configuration.showPercentage) {
            plasmoid.configuration.showPercentage = true;
        } else {
            plasmoid.configuration.showPercentage = false;
        }
    }

    Component.onCompleted: {
        Logic.updateBrightness(batterymonitor, pmSource);
        Logic.updateInhibitions(batterymonitor, pmSource)

        if (batterymonitor.kcmEnergyInformationAuthorized) {
            plasmoid.setAction("energyinformationkcm", i18n("&Show Energy Information…"), "documentinfo");
        }
        plasmoid.setAction("showPercentage", i18n("Show Battery Percentage on Icon"), "format-number-percent");
        plasmoid.action("showPercentage").checkable = true;
        plasmoid.action("showPercentage").checked = Qt.binding(() => plasmoid.configuration.showPercentage);

        if (batterymonitor.kcmAuthorized) {
            plasmoid.removeAction("configure");
            plasmoid.setAction("configure", i18n("&Configure Energy Saving…"), "configure", "alt+d, s");
        }
    }

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
            Logic.updateBrightness(batterymonitor, pmSource)
            Logic.updateInhibitions(batterymonitor, pmSource)
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

    Plasmoid.compactRepresentation: CompactRepresentation {}

    Plasmoid.fullRepresentation: PopupDialog {
        id: dialogItem
        Layout.minimumWidth: PlasmaCore.Units.iconSizes.medium * 9
        Layout.minimumHeight: PlasmaCore.Units.gridUnit * 15
        // TODO Probably needs a sensible preferredHeight too

        model: plasmoid.expanded ? batteries : null
        anchors.fill: parent
        focus: true

        isBrightnessAvailable: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Screen Brightness Available"] ? true : false
        isKeyboardBrightnessAvailable: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Keyboard Brightness Available"] ? true : false

        pluggedIn: pmSource.data["AC Adapter"] !== undefined && pmSource.data["AC Adapter"]["Plugged in"]

        readonly property string actuallyActiveProfile: pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Current Profile"] || "") : ""
        activeProfile: actuallyActiveProfile
        profiles: pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Profiles"] || []) : []
        inhibitionReason: pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Performance Inhibited Reason"] || "") : ""
        degradationReason: pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Performance Degraded Reason"] || "") : ""
        profileHolds:  pmSource.data["Power Profiles"] ? (pmSource.data["Power Profiles"]["Profile Holds"] || []) : []

        property int cookie1: -1
        property int cookie2: -1
        onPowermanagementChanged: {
            var service = pmSource.serviceForSource("PowerDevil");
            if (disabled) {
                var reason = i18n("The battery applet has enabled system-wide inhibition");
                var op1 = service.operationDescription("beginSuppressingSleep");
                op1.reason = reason;
                var op2 = service.operationDescription("beginSuppressingScreenPowerManagement");
                op2.reason = reason;

                var job1 = service.startOperationCall(op1);
                job1.finished.connect(function(job) {
                    cookie1 = job.result;
                });

                var job2 = service.startOperationCall(op2);
                job2.finished.connect(function(job) {
                    cookie2 = job.result;
                });
            } else {
                var op1 = service.operationDescription("stopSuppressingSleep");
                op1.cookie = cookie1;
                var op2 = service.operationDescription("stopSuppressingScreenPowerManagement");
                op2.cookie = cookie2;

                var job1 = service.startOperationCall(op1);
                job1.finished.connect(function(job) {
                    cookie1 = -1;
                });

                var job2 = service.startOperationCall(op2);
                job2.finished.connect(function(job) {
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
            dialogItem.activeProfile = profile
            const service = pmSource.serviceForSource("PowerDevil");
            let op = service.operationDescription("setPowerProfile");
            op.profile = profile;

            let job = service.startOperationCall(op);
            job.finished.connect((job) => {
                dialogItem.activeProfile = Qt.binding(() => actuallyActiveProfile)
                if (!job.result) {
                    var notifications = notificationSource.serviceForSource("notification")
                    var operation = notifications.operationDescription("createNotification");
                    operation.appName = i18n("Battery and Brightness")
                    operation.appIcon = "dialog-error";
                    operation.icon = "dialog-error"
                    operation.body = i18n("Failed to activate %1 mode", profile)
                    notifications.startOperationCall(operation);
                }
            });
        }
    }
}
