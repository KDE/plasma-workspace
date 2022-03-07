/*
    SPDX-FileCopyrightText: 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
    SPDX-FileCopyrightText: 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.kcoreaddons 1.0 as KCoreAddons
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.workspace.components 2.0

import "logic.js" as Logic

RowLayout {
    id: root

    // We'd love to use `required` properties, especially since the model provides role names for them;
    // but unfortunately some of those roles have whitespaces in their name, which QML doesn't have any
    // workaround for (raw identifiers like r#try in Rust would've helped here).
    //
    // type: {
    //  Capacity:           int,
    //  Energy:             real,
    //  "Is Power Supply":  bool,
    //  Percent:            int,
    //  "Plugged In":       bool,
    //  "Pretty Name":      string,
    //  Product:            string,
    //  State:              "Discharging"|"Charging"|"FullyCharged"|etc.,
    //  Type:               string,
    //  Vendor:             string,
    // }?
    property var battery

    // NOTE: According to the UPower spec this property is only valid for primary batteries, however
    // UPower seems to set the Present property false when a device is added but not probed yet
    readonly property bool isPresent: root.battery["Plugged in"]

    readonly property bool isPowerSupply: root.battery["Is Power Supply"]

    readonly property bool isBroken: root.battery.Capacity > 0 && root.battery.Capacity < 50

    property int remainingTime: 0

    // Existing instance of a slider to use as a reference to calculate extra
    // margins for a progress bar, so that the row of labels on top of it
    // could visually look as if it were on the same distance from the bar as
    // they are from the slider.
    property PlasmaComponents3.Slider matchHeightOfSlider: PlasmaComponents3.Slider {}
    readonly property real extraMargin: Math.max(0, Math.floor((matchHeightOfSlider.height - chargeBar.height) / 2))

    spacing: PlasmaCore.Units.gridUnit

    BatteryIcon {
        id: batteryIcon

        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: PlasmaCore.Units.iconSizes.medium
        Layout.preferredHeight: PlasmaCore.Units.iconSizes.medium

        batteryType: root.battery.Type
        percent: root.battery.Percent
        hasBattery: root.isPresent
        pluggedIn: root.battery.State === "Charging" && root.battery["Is Power Supply"]
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: root.isPresent ? Qt.AlignTop : Qt.AlignVCenter
        spacing: 0

        RowLayout {
            spacing: PlasmaCore.Units.smallSpacing

            PlasmaComponents3.Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: root.battery["Pretty Name"]
            }

            PlasmaComponents3.Label {
                text: Logic.stringForBatteryState(root.battery)
                visible: root.battery["Is Power Supply"]
                enabled: false
            }

            PlasmaComponents3.Label {
                horizontalAlignment: Text.AlignRight
                visible: root.isPresent
                text: i18nc("Placeholder is battery percentage", "%1%", root.battery.Percent)
            }
        }

        PlasmaComponents3.ProgressBar {
            id: chargeBar

            Layout.fillWidth: true
            Layout.topMargin: root.extraMargin
            Layout.bottomMargin: root.extraMargin

            from: 0
            to: 100
            visible: root.isPresent
            value: Number(root.battery.Percent)
        }

        // This gridLayout basically emulates an at-most-two-rows table with a
        // single wide fillWidth/columnSpan header. Not really worth it trying
        // to refactor it into some more clever fancy model-delegate stuff.
        GridLayout {
            id: details

            Layout.fillWidth: true
            Layout.topMargin: PlasmaCore.Units.smallSpacing

            columns: 2
            columnSpacing: PlasmaCore.Units.smallSpacing
            rowSpacing: 0

            component LeftLabel : PlasmaComponents3.Label {
                // fillWidth is true, so using internal alignment
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
                font: PlasmaCore.Theme.smallestFont
                wrapMode: Text.WordWrap
                enabled: false
            }
            component RightLabel : PlasmaComponents3.Label {
                // fillWidth is false, so using external (grid-cell-internal) alignment
                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: false
                font: PlasmaCore.Theme.smallestFont
                enabled: false
            }

            PlasmaComponents3.Label {
                Layout.fillWidth: true
                Layout.columnSpan: 2

                text: root.isBroken && typeof root.battery.Capacity !== "undefined"
                    ? i18n("This battery's health is at only %1% and it should be replaced. Contact the manufacturer.", root.battery.Capacity)
                    : ""
                font: PlasmaCore.Theme.smallestFont
                color: PlasmaCore.Theme.neutralTextColor
                visible: root.isBroken
                wrapMode: Text.WordWrap
            }

            readonly property bool remainingTimeRowVisible: root.battery !== null
                && root.remainingTime > 0
                && root.battery["Is Power Supply"]
                && ["Discharging", "Charging"].includes(root.battery.State)

            LeftLabel {
                text: root.battery.State === "Charging"
                    ? i18n("Time To Full:")
                    : i18n("Remaining Time:")
                visible: details.remainingTimeRowVisible
            }

            RightLabel {
                text: KCoreAddons.Format.formatDuration(root.remainingTime, KCoreAddons.FormatTypes.HideSeconds)
                visible: details.remainingTimeRowVisible
            }

            readonly property bool healthRowVisible: root.battery !== null
                && root.battery["Is Power Supply"]
                && root.battery.Capacity !== ""
                && typeof root.battery.Capacity === "number"
                && !root.isBroken

            LeftLabel {
                text: i18n("Battery Health:")
                visible: details.healthRowVisible
            }

            RightLabel {
                text: details.healthRowVisible
                    ? i18nc("Placeholder is battery health percentage", "%1%", root.battery.Capacity)
                    : ""
                visible: details.healthRowVisible
            }
        }

        InhibitionHint {
            Layout.fillWidth: true
            Layout.topMargin: PlasmaCore.Units.smallSpacing

            readonly property var chargeStopThreshold: pmSource.data["Battery"] ? pmSource.data["Battery"]["Charge Stop Threshold"] : undefined
            readonly property bool pluggedIn: pmSource.data["AC Adapter"] !== undefined && pmSource.data["AC Adapter"]["Plugged in"]
            visible: pluggedIn && root.isPowerSupply && typeof chargeStopThreshold === "number" && chargeStopThreshold > 0 && chargeStopThreshold < 100
            iconSource: "kt-speed-limits" // FIXME good icon
            text: i18n("Your battery is configured to only charge up to %1%.", chargeStopThreshold || 0)
        }
    }
}
