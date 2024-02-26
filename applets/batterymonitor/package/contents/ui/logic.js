/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2012 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2014-2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

function stringForBatteryState(pluggedIn, chargeStopThreshold, percent, state, batteryState) {
    if (pluggedIn) {
        // When we are using a charge threshold, the kernel
        // may stop charging within a percentage point of the actual threshold
        // and this is considered correct behavior, so we have to handle
        // that. See https://bugzilla.kernel.org/show_bug.cgi?id=215531.
        if ( (percent >= chargeStopThreshold - 1
            && percent <= chargeStopThreshold + 1)
            // Also, Upower may give us a status of "Not charging" rather than
            // "Fully charged", so we need to account for that as well. See
            // https://gitlab.freedesktop.org/upower/upower/-/issues/142.
            && (state === "NoCharge" || state === "FullyCharged")
        ) {
            return i18n("Fully Charged");
        }

        // Otherwise, just look at the charge state
        switch(batteryState) {
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
