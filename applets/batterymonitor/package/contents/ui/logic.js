/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2012 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2014-2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

function stringForBatteryState(batteryData, source) {
    if (batteryData["Plugged in"]) {
        // When we are using a charge threshold, the kernel
        // may stop charging within a percentage point of the actual threshold
        // and this is considered correct behavior, so we have to handle
        // that. See https://bugzilla.kernel.org/show_bug.cgi?id=215531.
        if (typeof source.data["Battery"]["Charge Stop Threshold"] === "number"
            && (source.data.Battery.Percent >= source.data["Battery"]["Charge Stop Threshold"] - 1
            && source.data.Battery.Percent <= source.data["Battery"]["Charge Stop Threshold"] + 1)
            // Also, Upower may give us a status of "Not charging" rather than
            // "Fully charged", so we need to account for that as well. See
            // https://gitlab.freedesktop.org/upower/upower/-/issues/142.
            && (source.data["Battery"]["State"] === "NoCharge" || source.data["Battery"]["State"] === "FullyCharged")
        ) {
            return i18n("Fully Charged");
        }

        // Otherwise, just look at the charge state
        switch(batteryData["State"]) {
            case "Discharging": return i18n("Discharging");
            case "FullyCharged": return i18n("Fully Charged");
            case "Charging": return i18n("Charging");
            // when in doubt we're not charging
            default: return i18n("Not Charging");
        }
    } else {
        return i18nc("Battery is currently not present in the bay", "Not present");
    }
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
    const inhibitions = [];

    if (source.data["Inhibitions"]) {
        for (let key in pmSource.data["Inhibitions"]) {
            if (key === "plasmashell" || key === "plasmoidviewer") { // ignore our own inhibition
                continue;
            }

            inhibitions.push(pmSource.data["Inhibitions"][key]);
        }
    }

    rootItem.inhibitions = inhibitions;
}
