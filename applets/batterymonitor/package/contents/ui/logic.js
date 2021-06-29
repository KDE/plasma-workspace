/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2012 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2014-2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

function stringForBatteryState(batteryData) {
    if (batteryData["Plugged in"]) {
        switch(batteryData["State"]) {
            case "Discharging": return i18n("Discharging");
            case "FullyCharged": return i18n("Fully Charged");
            case "Charging": return i18n("Charging");
            // when in doubt we're not charging
            default: return i18n("Not Charging");
        }
    } else {
        return i18nc("Battery is currently not present in the bay","Not present");
    }
}

function batteryDetails(batteryData, remainingTime) {
    var data = []

    if (remainingTime > 0 && batteryData["Is Power Supply"] && (batteryData["State"] == "Discharging" || batteryData["State"] == "Charging")) {
        data.push({label: (batteryData["State"] == "Charging" ? i18n("Time To Full:") : i18n("Remaining Time:")) })
        data.push({value: KCoreAddons.Format.formatDuration(remainingTime, KCoreAddons.FormatTypes.HideSeconds) })
    }

    if (batteryData["Is Power Supply"] && batteryData["Capacity"] != "" && typeof batteryData["Capacity"] == "number") {
        data.push({label: i18n("Battery Health:") })
        data.push({value: i18nc("Placeholder is battery health percentage", "%1%", batteryData["Capacity"]) })
    }

    return data
}

function updateBrightness(rootItem, source) {
    if (rootItem.updateScreenBrightnessJob || rootItem.updateKeyboardBrightnessJob)
        return;

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

function updateInhibitions(rootItem, source) {
    var inhibitions = [];

    if (source.data["Inhibitions"]) {
        for(var key in pmSource.data["Inhibitions"]) {
            if (key === "plasmashell" || key === "plasmoidviewer") { // ignore our own inhibition
                continue
            }

            inhibitions.push(pmSource.data["Inhibitions"][key])
        }
    }

    rootItem.inhibitions = inhibitions
}
