/*
 *   Copyright 2011 Sebastian Kügler <sebas@kde.org>
 *   Copyright 2012 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
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

var powermanagementDisabled = false;

function updateCumulative() {
    var count = 0;
    var energy = 0;
    var totalEnergy = 0;
    var charged = true;
    var plugged = false;
    for (var i=0; i<batteries.count; i++) {
        var b = batteries.get(i);
        if (!b["Is Power Supply"]) {
          continue;
        }
        if (b["Plugged in"]) {
            energy += b["Energy"];
            totalEnergy += b["Energy"] / (b["Percent"] / 100) * (b["Capacity"]/100);
            plugged = true;
        }
        if (b["State"] != "FullyCharged") {
            charged = false;
        }
        count++;
    }

    if (count > 0) {
      batteries.cumulativePercent = Math.round(energy/totalEnergy*100);
    } else {
        // We don't have any power supply batteries
        // Use the lowest value from any battery
        if (batteries.count > 0) {
            var b = lowestBattery();
            batteries.cumulativePercent = b["Percent"];
        } else {
            batteries.cumulativePercent = 0;
        }
    }
    batteries.cumulativePluggedin = plugged;
    batteries.allCharged = charged;
}

function plasmoidStatus() {
    var status = PlasmaCore.Types.PassiveStatus;
    if (powermanagementDisabled) {
        status = PlasmaCore.Types.ActiveStatus;
    }

    if (batteries.cumulativePluggedin) {
        if (!batteries.allCharged) {
            status = PlasmaCore.Types.ActiveStatus;
        }
    } else if (batteries.count > 0) { // in case your mouse gets low
        if (batteries.cumulativePercent && batteries.cumulativePercent <= 10) {
            status = PlasmaCore.Types.NeedsAttentionStatus;
        }
    }
    return status;
}

function lowestBattery() {
    if (batteries.count == 0) {
        return;
    }

    var lowestPercent = 100;
    var lowestBattery;
    for(var i=0; i<batteries.count; i++) {
        var b = batteries.get(i);
        if (b["Percent"] && b["Percent"] < lowestPercent) {
            lowestPercent = b["Percent"];
            lowestBattery = b;
        }
    }
    return b;
}

function stringForBatteryState(batteryData) {
    if (batteryData["Plugged in"]) {
        switch(batteryData["State"]) {
            case "NoCharge": return i18n("Not Charging");
            case "Discharging": return i18n("Discharging");
            case "FullyCharged": return i18n("Fully Charged");
            default: return i18n("Charging");
        }
    } else {
        return i18nc("Battery is currently not present in the bay","Not present");
    }
}

function updateTooltip(remainingTime) {
    if (powermanagementDisabled) {
        batteries.tooltipSubText = i18n("Power management is disabled");
    } else {
        batteries.tooltipSubText = "";
    }

    if (batteries.count == 0) {
        batteries.tooltipImage = "battery-missing";
        batteries.tooltipMainText = i18n("No Batteries Available");
    } else if (batteries.allCharged) {
        batteries.tooltipImage = "battery-100";
        batteries.tooltipMainText = i18n("Fully Charged");
    } else if (pmSource.data["AC Adapter"] && pmSource.data["AC Adapter"]["Plugged in"]) {
        batteries.tooltipImage = "battery-charging"
        batteries.tooltipMainText = i18n("%1%. Charging", batteries.cumulativePercent);
    } else {
        batteries.tooltipImage = "battery-discharging"

        if (remainingTime > 0) {
            batteries.tooltipMainText = i18nc("%1 is remaining time, %2 is percentage", "%1 Remaining (%2%)",
                                              KCoreAddons.Format.formatDuration(remainingTime, KCoreAddons.FormatTypes.HideSeconds),
                                              batteries.cumulativePercent)
        } else {
            batteries.tooltipMainText = i18n("%1% Battery Remaining", batteries.cumulativePercent);
        }
    }
}

function batteryDetails(batteryData, remainingTime) {
    var data = []

    if (remainingTime > 0 && batteryData["Is Power Supply"] && (batteryData["State"] == "Discharging" || batteryData["State"] == "Charging")) {
        data.push({label: (batteryData["State"] == "Charging" ? i18n("Time To Full:") : i18n("Time To Empty:")) })
        data.push({value: KCoreAddons.Format.formatDuration(remainingTime, KCoreAddons.FormatTypes.HideSeconds) })
    }

    if (batteryData["Is Power Supply"] && batteryData["Capacity"] != "" && typeof batteryData["Capacity"] == "number") {
        data.push({label: i18n("Capacity:") })
        data.push({value: i18nc("Placeholder is battery capacity", "%1%", batteryData["Capacity"]) })
    }

    // Non-powersupply batteries have a name consisting of the vendor and model already
    if (batteryData["Is Power Supply"]) {
        if (batteryData["Vendor"] != "" && typeof batteryData["Vendor"] == "string") {
            data.push({label: i18n("Vendor:") })
            data.push({value: batteryData["Vendor"] })
        }

        if (batteryData["Product"] != "" && typeof batteryData["Product"] == "string") {
            data.push({label: i18n("Model:") })
            data.push({value: batteryData["Product"] })
        }
    }

    return data
}

function updateBrightness(rootItem, source) {
    if (!source.data["PowerDevil"]) {
        return;
    }

    // we don't want passive brightness change send setBrightness call
    rootItem.disableBrightnessUpdate = true;

    if (typeof source.data["PowerDevil"]["Screen Brightness"] === 'number') {
        rootItem.screenBrightness = source.data["PowerDevil"]["Screen Brightness"];
    }
    if (typeof source.data["PowerDevil"]["Keyboard Brightness"] === 'number') {
        rootItem.keyboardBrightness = source.data["PowerDevil"]["Keyboard Brightness"];
    }
    rootItem.disableBrightnessUpdate = false;
}
