/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.coreaddons as KCoreAddons
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.workspace.components as PW
import org.kde.kirigami as Kirigami

import org.kde.plasma.private.battery

RowLayout {
    id: root

    property real fontSize: Kirigami.Theme.defaultFont.pointSize

    BatteryControlModel {
        id: batteryControl
    }

    spacing: Kirigami.Units.smallSpacing
    visible: batteryControl.hasInternalBatteries

    HoverHandler { id: hoverHandler }

    PW.BatteryIcon {
        pluggedIn: batteryControl.pluggedIn
        hasBattery: batteryControl.hasCumulative
        percent: batteryControl.percent

        Layout.preferredHeight: Math.max(Kirigami.Units.iconSizes.medium, batteryLabel.implicitHeight)
        Layout.preferredWidth: Layout.preferredHeight
        Layout.alignment: Qt.AlignVCenter
    }

    PlasmaComponents3.Label {
        id: batteryLabel
        font.pointSize: root.fontSize
        text: i18nd("plasma_applet_org.kde.plasma.battery", "%1%", batteryControl.percent)
        textFormat: Text.PlainText
        Accessible.name: i18nd("plasma_applet_org.kde.plasma.battery", "Battery at %1%", batteryControl.percent)
        Layout.alignment: Qt.AlignVCenter
    }

    PlasmaComponents3.ToolTip {
        text: {
            const percent = batteryControl.percent;
            let batteryState = i18nd("plasma_applet_org.kde.plasma.battery", "Battery at %1%", percent);

            if (batteryControl.pluggedIn) {
                const state = batteryControl.state;
                if (state === BatteryControlModel.NoCharge) {
                    batteryState = i18nd("plasma_applet_org.kde.plasma.battery", "Battery at %1%, not Charging", percent);
                } else if (state === BatteryControlModel.Discharging) {
                    batteryState = i18nd("plasma_applet_org.kde.plasma.battery", "Battery at %1%, plugged in but still discharging", percent);
                } else if (state === BatteryControlModel.Charging) {
                    batteryState = i18nd("plasma_applet_org.kde.plasma.battery", "Battery at %1%, Charging", percent);
                }
            }

            const parts = [batteryState];

            if (batteryControl.smoothedRemainingMsec > 0) {
                const remainingTimeString = KCoreAddons.Format.formatDuration(batteryControl.smoothedRemainingMsec, KCoreAddons.FormatTypes.HideSeconds | KCoreAddons.FormatTypes.AbbreviatedDuration);

                if (batteryControl.state === BatteryControlModel.FullyCharged) {
                    // Don't add anything
                } else if (batteryControl.pluggedIn && batteryControl.state === BatteryControlModel.Charging) {
                    parts.push(i18ndc("plasma_applet_org.kde.plasma.battery", "time until fully charged - HH:MM", "%1 until fully charged", remainingTimeString));
                } else {
                    parts.push(i18ndc("plasma_applet_org.kde.plasma.battery", "remaining time left of battery usage - HH:MM", "%1 remaining", remainingTimeString));
                }
            }

            return parts.join("\n\n");
        }
        visible: text.length > 0 && hoverHandler.hovered
        timeout: -1
    }
}
