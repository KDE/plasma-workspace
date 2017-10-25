/*
 *   Copyright 2011 Sebastian Kügler <sebas@kde.org>
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kcoreaddons 1.0 as KCoreAddons
import org.kde.kquickcontrolsaddons 2.0
import "logic.js" as Logic

Item {
    id: batterymonitor
    Plasmoid.switchWidth: units.gridUnit * 10
    Plasmoid.switchHeight: units.gridUnit * 10

    LayoutMirroring.enabled: Qt.application.layoutDirection == Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Plasmoid.status: {
        if (powermanagementDisabled) {
            return PlasmaCore.Types.ActiveStatus
        }

        if (pmSource.data.Battery["Has Cumulative"]) {
            if (pmSource.data.Battery.State !== "Charging" && pmSource.data.Battery.Percent <= 5) {
                return PlasmaCore.Types.NeedsAttentionStatus
            } else if (pmSource.data["Battery"]["State"] !== "FullyCharged") {
                return PlasmaCore.Types.ActiveStatus
            }
        }

        return PlasmaCore.Types.PassiveStatus
    }

    Plasmoid.toolTipMainText: {
        if (batteries.count === 0) {
            return i18n("No Batteries Available");
        } else if (!pmSource.data["Battery"]["Has Cumulative"]) {
            // Bug 362924: Distinguish between no batteries and no power supply batteries
            // just show the generic applet title in the latter case
            return i18n("Battery and Brightness")
        } else if (pmSource.data["Battery"]["State"] === "FullyCharged") {
            return i18n("Fully Charged");
        } else if (pmSource.data["AC Adapter"] && pmSource.data["AC Adapter"]["Plugged in"]) {
            var percent = pmSource.data.Battery.Percent
            var state = pmSource.data.Battery.State
            if (state === "Charging") {
                return i18n("%1%. Charging", percent)
            } else if (state === "NoCharge") {
                return i18n("%1%. Plugged in, not Charging", percent)
            } else {
                return i18n("%1%. Plugged in", percent)
            }
        } else {
            if (remainingTime > 0) {
                return i18nc("%1 is remaining time, %2 is percentage", "%1 Remaining (%2%)",
                             KCoreAddons.Format.formatDuration(remainingTime, KCoreAddons.FormatTypes.HideSeconds),
                             pmSource.data["Battery"]["Percent"])
            } else {
                return i18n("%1% Battery Remaining", pmSource.data["Battery"]["Percent"]);
            }
        }
    }
    Plasmoid.toolTipSubText: powermanagementDisabled ? i18n("Power management is disabled") : ""
    Plasmoid.icon: "battery"

    property bool disableBrightnessUpdate: true

    property int screenBrightness
    readonly property int maximumScreenBrightness: pmSource.data["PowerDevil"] ? pmSource.data["PowerDevil"]["Maximum Screen Brightness"] || 0 : 0

    property int keyboardBrightness
    readonly property int maximumKeyboardBrightness: pmSource.data["PowerDevil"] ? pmSource.data["PowerDevil"]["Maximum Keyboard Brightness"] || 0 : 0

    readonly property int remainingTime: Number(pmSource.data["Battery"]["Remaining msec"])

    property bool powermanagementDisabled: false

    property var inhibitions: []

    readonly property var kcms: ["powerdevilprofilesconfig.desktop",
                                 "powerdevilactivitiesconfig.desktop",
                                 "powerdevilglobalconfig.desktop"]
    readonly property bool kcmsAuthorized: KCMShell.authorize(batterymonitor.kcms).length > 0

    onScreenBrightnessChanged: {
        if (disableBrightnessUpdate) {
            return;
        }
        var service = pmSource.serviceForSource("PowerDevil");
        var operation = service.operationDescription("setBrightness");
        operation.brightness = screenBrightness;
        // show OSD only when the plasmoid isn't expanded since the moving slider is feedback enough
        operation.silent = plasmoid.expanded
        service.startOperationCall(operation);
    }
    onKeyboardBrightnessChanged: {
        if (disableBrightnessUpdate) {
            return;
        }
        var service = pmSource.serviceForSource("PowerDevil");
        var operation = service.operationDescription("setKeyboardBrightness");
        operation.brightness = keyboardBrightness;
        operation.silent = plasmoid.expanded
        service.startOperationCall(operation);
    }

    function action_powerdevilkcm() {
        KCMShell.open(batterymonitor.kcms);
    }

    Component.onCompleted: {
        Logic.updateBrightness(batterymonitor, pmSource);
        Logic.updateInhibitions(batterymonitor, pmSource)

        if (batterymonitor.kcmsAuthorized) {
            plasmoid.setAction("powerdevilkcm", i18n("&Configure Power Saving..."), "preferences-system-power-management");
        }
    }

    Plasmoid.compactRepresentation: CompactRepresentation {
        property int wheelDelta: 0

        onEntered: wheelDelta = 0
        onExited: wheelDelta = 0
        onWheel: {
            var delta = wheel.angleDelta.y || wheel.angleDelta.x

            var maximumBrightness = batterymonitor.maximumScreenBrightness
            // Don't allow the UI to turn off the screen
            // Please see https://git.reviewboard.kde.org/r/122505/ for more information
            var minimumBrightness = (maximumBrightness > 100 ? 1 : 0)
            var steps = Math.max(1, Math.round(maximumBrightness / 20))
            var deltaSteps = delta / 120;
            batterymonitor.screenBrightness = Math.max(minimumBrightness, Math.min(maximumBrightness, batterymonitor.screenBrightness + deltaSteps * steps));
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

    Plasmoid.fullRepresentation: PopupDialog {
        id: dialogItem
        Layout.minimumWidth: units.iconSizes.medium * 9
        Layout.minimumHeight: units.gridUnit * 15
        // TODO Probably needs a sensible preferredHeight too

        model: plasmoid.expanded ? batteries : null
        anchors.fill: parent
        focus: true

        isBrightnessAvailable: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Screen Brightness Available"] ? true : false
        isKeyboardBrightnessAvailable: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Keyboard Brightness Available"] ? true : false

        pluggedIn: pmSource.data["AC Adapter"] != undefined && pmSource.data["AC Adapter"]["Plugged in"]

        property int cookie1: -1
        property int cookie2: -1
        onPowermanagementChanged: {
            var service = pmSource.serviceForSource("PowerDevil");
            if (checked) {
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
            } else {
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
            }
            batterymonitor.powermanagementDisabled = !checked
        }
    }
}
