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
    var sum = 0;
    var count = 0;
    var charged = true;
    var plugged = false;
    for (var i=0; i<batteries.count; i++) {
        var b = batteries.get(i);
        if (!b["Is Power Supply"]) {
          continue;
        }
        if (b["Plugged in"]) {
            sum += b["Percent"];
            plugged = true;
        }
        if (b["State"] != "FullyCharged") {
            charged = false;
        }
        count++;
    }

    if (count > 0) {
      batteries.cumulativePercent = Math.round(sum/count);
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
            case "NoCharge": return "" // FIXME string freeze return i18n("Not Charging");
            case "Discharging": return i18n("Discharging");
            case "FullyCharged": return i18n("Fully Charged");
            default: return i18n("Charging");
        }
    } else {
        return i18nc("Battery is currently not present in the bay","Not present");
    }
}

function iconForBattery(batteryData,pluggedIn) {
    switch(batteryData["Type"]) {
        case "Mouse":
            return "input-mouse-battery";
        case "Keyboard":
            return "input-keyboard-battery";
        case "Pda":
            return "phone-battery";
        case "Phone":
            return "phone-battery";
        case "UPS":
            return "battery-ups";
        default: // Primary and UPS
            var p = batteryData["Percent"];

            var fill;
            if (p >= 90) {
                fill = "100";
            } else if (p >= 70) {
                fill = "080";
            } else if (p >= 50) {
                fill = "060";
            } else if (p >= 30) {
                fill = "040";
            } else if (p >= 10) {
                fill = "caution";
            } else {
                fill = "low";
            }

            if (pluggedIn && batteryData["Is Power Supply"]) {
                return "battery-charging-" + fill;
            } else {
                if (p <= 5) {
                    return "dialog-warning"
                }
                return "battery-" + fill;
            }
    }
}

function updateTooltip() {
    var image = "";
    var text = "";
    if (batteries.count == 0) {
        image = "battery-missing";
        if (!powermanagementDisabled) {
            text = i18n("No batteries available");
        }
    } else {
        var hasPowerSupply = false;

        text = "<table style='white-space: nowrap'>";
        for(var i=0; i<batteries.count; i++) {
            var b = batteries.get(i);
            text += "<tr>";
            text += "<td align='right'>" + i18nc("Placeholder is battery name", "%1:", b["Pretty Name"]) + " </td>";
            text += "<td><b>";
            if (b["Plugged in"]) {
                text += i18nc("Placeholder is battery percentage", "%1%", b["Percent"]);
            } else {
                text += i18n("N/A")
            }
            text += "</b></td>";
            text += "</tr>";

            if (b["Is Power Supply"]) { hasPowerSupply = true; }
        }
        text += "</table>";

        if (hasPowerSupply) {
            var b = [];
            b["Type"] = "Battery";
            b["Percent"] = batteries.cumulativePercent;
            if (pmSource.data["AC Adapter"] !== undefined) {
                image = iconForBattery(b, pmSource.data["AC Adapter"]["Plugged in"] ? true : false);
            }
        } else {
            var b = lowestBattery();
            image = iconForBattery(b, false);
        }
    }

    if (powermanagementDisabled) {
        text += i18n("Power management is disabled");
    }
    batteries.tooltipText = text;
    batteries.tooltipImage = image;
}

function batteryItemToolTip(batteryData, remainingTime) {
    var text = "";

    if (remainingTime > 0 && batteryData["Is Power Supply"] && (batteryData["State"] == "Discharging" || batteryData["State"] == "Charging")) {
        text += "<tr>"
        text += "<td align='right'>" + (batteryData["State"] == "Charging" ? i18n("Time To Full:") : i18n("Time To Empty:")) + "</td>"
        text += "<td><b>" + KCoreAddons.Format.formatSpelloutDuration(remainingTime) + "</b></td>"
        text += "</tr>"
    }

    if (batteryData["Is Power Supply"] &&  batteryData["Capacity"] != "" && typeof batteryData["Capacity"] == "number") {
        text += "<tr>";
        text += "<td align='right'>" + i18n("Capacity:") + " </td>";
        text += "<td><b>" + i18nc("Placeholder is battery capacity", "%1%", batteryData["Capacity"]) + "</b></td>"
        text += "</tr>";
    }

    // Non-powersupply batteries have a name consisting of the vendor and model already
    if (batteryData["Is Power Supply"]) {
        if (batteryData["Vendor"] != "" && typeof batteryData["Vendor"] == "string") {
            text += "<tr>";
            text += "<td align='right'>" + i18n("Vendor:") + " </td>";
            text += "<td><b>" + batteryData["Vendor"] + "</b></td>";
            text += "</tr>";
        }

        if (batteryData["Product"] != "" && typeof batteryData["Product"] == "string") {
            text += "<tr>";
            text += "<td align='right'>" + i18n("Model:") + " </td>";
            text += "<td><b>" + batteryData["Product"] + "</b></td>";
            text += "</tr>";
        }
    }

    if (text != "") {
        return "<table>" + text + "</table>";
    }
    return "";
}

function updateBrightness(dialogItem, source) {
    if (!source.data["PowerDevil"]) {
        return;
    }

    // we don't want passive brightness change send setBrightness call
    dialogItem.disableBrightnessUpdate = true;

    if (typeof source.data["PowerDevil"]["Screen Brightness"] === 'number') {
        dialogItem.screenBrightness = source.data["PowerDevil"]["Screen Brightness"];
        dialogItem.screenBrightnessPercentage = source.data["PowerDevil"]["Screen Brightness"];
    }
    if (typeof source.data["PowerDevil"]["Keyboard Brightness"] === 'number') {
        dialogItem.keyboardBrightness = source.data["PowerDevil"]["Keyboard Brightness"];
        dialogItem.keyboardBrightnessPercentage = source.data["PowerDevil"]["Keyboard Brightness"];
    }
    dialogItem.disableBrightnessUpdate = false;
}
