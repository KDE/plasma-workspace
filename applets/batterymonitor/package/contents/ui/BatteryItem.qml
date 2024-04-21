/*
    SPDX-FileCopyrightText: 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
    SPDX-FileCopyrightText: 2013-2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.coreaddons as KCoreAddons
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.workspace.components
import org.kde.kirigami as Kirigami

import org.kde.plasma.private.battery


PlasmaComponents3.ItemDelegate {
    id: root

    property int batteryPercent: 0

    property int batteryCapacity: 0

    property real batteryEnergy: 0.0

    // NOTE: According to the UPower spec this property is only valid for primary batteries, however
    // UPower seems to set the Present property false when a device is added but not probed yet
    property bool batteryPluggedIn: false

    property bool batteryIsPowerSupply: false

    property int batteryChargeState: 0

    property string batteryPrettyName: ""

    property string batteryType: ""


    readonly property bool isBroken: root.batteryCapacity > 0 && root.batteryCapacity < 50

    property bool pluggedIn: false

    property int remainingTime: 0

    property int chargeStopThreshold: 0

    // Existing instance of a slider to use as a reference to calculate extra
    // margins for a progress bar, so that the row of labels on top of it
    // could visually look as if it were on the same distance from the bar as
    // they are from the slider.
    property PlasmaComponents3.Slider matchHeightOfSlider: PlasmaComponents3.Slider {}
    readonly property real extraMargin: Math.max(0, Math.floor((matchHeightOfSlider.height - chargeBar.height) / 2))

    background.visible: highlighted
    highlighted: activeFocus
    hoverEnabled: false
    text: batteryPrettyName

    Accessible.description: `${isPowerSupplyLabel.text} ${percentLabel.text}; ${details.Accessible.description}`

    contentItem: RowLayout {
        spacing: Kirigami.Units.gridUnit

        BatteryIcon {
            id: batteryIcon

            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: Kirigami.Units.iconSizes.medium
            
            batteryType: root.batteryType
            percent: root.batteryPercent
            hasBattery: root.batteryPluggedIn
            pluggedIn: root.pluggedIn && root.batteryIsPowerSupply
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: root.batteryPluggedIn ? Qt.AlignTop : Qt.AlignVCenter
            spacing: 0

            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: root.text
                    textFormat: Text.PlainText
                }

                PlasmaComponents3.Label {
                    id: isPowerSupplyLabel
                    text: {
                        if(batteryPluggedIn) {
                            switch (root.batteryChargeState) {
                                case BatteryControlModel.Charging:
                                    return i18n("Charging");
                                case BatteryControlModel.Discharging:
                                    return i18n("Discharging");
                                case BatteryControlModel.FullyCharged:
                                    return i18n("Fully Charged");
                                default:
                                    return i18n("Not Charging");
                            }
                        }
                            return i18nc("Battery is currently not present in the bay", "Not present");
                    }
                    textFormat: Text.PlainText
                    // For non-power supply batteries only show label for known-good states
                    visible: root.batteryIsPowerSupply || root.batteryChargeState !== BatteryControlModel.NoCharge
                    enabled: false
                }

                PlasmaComponents3.Label {
                    id: percentLabel
                    horizontalAlignment: Text.AlignRight
                    visible: root.batteryPluggedIn
                    text: i18nc("Placeholder is battery percentage", "%1%", root.batteryPercent)
                    textFormat: Text.PlainText
                }
            }

            PlasmaComponents3.ProgressBar {
                id: chargeBar

                Layout.fillWidth: true
                Layout.topMargin: root.extraMargin
                Layout.bottomMargin: root.extraMargin

                from: 0
                to: 100
                visible: root.batteryPluggedIn
                value: root.batteryPercent
            }

            // This gridLayout basically emulates an at-most-two-rows table with a
            // single wide fillWidth/columnSpan header. Not really worth it trying
            // to refactor it into some more clever fancy model-delegate stuff.
            GridLayout {
                id: details

                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing

                columns: 2
                columnSpacing: Kirigami.Units.smallSpacing
                rowSpacing: 0

                Accessible.description: {
                    let description = [];
                    for (let i = 0; i < children.length; i++) {
                        if (children[i].visible && children[i].hasOwnProperty("text")) {
                            description.push(children[i].text);
                        }
                    }
                    return description.join(" ");
                }

                component LeftLabel : PlasmaComponents3.Label {
                    // fillWidth is true, so using internal alignment
                    horizontalAlignment: Text.AlignLeft
                    Layout.fillWidth: true
                    font: Kirigami.Theme.smallFont
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                    enabled: false
                }
                component RightLabel : PlasmaComponents3.Label {
                    // fillWidth is false, so using external (grid-cell-internal) alignment
                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: false
                    font: Kirigami.Theme.smallFont
                    enabled: false
                    textFormat: Text.PlainText
                }

                PlasmaComponents3.Label {
                    Layout.fillWidth: true
                    Layout.columnSpan: 2

                    text: root.isBroken
                        ? i18n("This battery's health is at only %1% and it should be replaced. Contact the manufacturer.", root.batteryCapacity)
                        : ""
                    textFormat: Text.PlainText
                    font: Kirigami.Theme.smallFont
                    color: Kirigami.Theme.neutralTextColor
                    visible: root.isBroken
                    wrapMode: Text.WordWrap
                }

                readonly property bool remainingTimeRowVisible: root.remainingTime > 0
                    && root.batteryIsPowerSupply
                    && [BatteryControlModel.Discharging, BatteryControlModel.Charging].includes(root.batteryChargeState)

                readonly property bool isEstimatingRemainingTime: root.batteryIsPowerSupply
                    && root.remainingTime === 0
                    && root.batteryChargeState === BatteryControlModel.Discharging

                LeftLabel {
                    text: root.batteryChargeState === BatteryControlModel.Charging
                        ? i18n("Time To Full:")
                        : i18n("Remaining Time:")

                    visible: details.remainingTimeRowVisible || details.isEstimatingRemainingTime
                }

                RightLabel {
                    text: details.isEstimatingRemainingTime ? i18nc("@info", "Estimating…")
                        : KCoreAddons.Format.formatDuration(root.remainingTime, KCoreAddons.FormatTypes.HideSeconds)
                    visible: details.remainingTimeRowVisible || details.isEstimatingRemainingTime
                }

                readonly property bool healthRowVisible: root.batteryIsPowerSupply
                    && root.batteryCapacity !== ""
                    && !root.isBroken

                LeftLabel {
                    text: i18n("Battery Health:")
                    visible: details.healthRowVisible
                }

                RightLabel {
                    text: details.healthRowVisible
                        ? i18nc("Placeholder is battery health percentage", "%1%", root.batteryCapacity)
                        : ""
                    visible: details.healthRowVisible
                }
            }

            InhibitionHint {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing

                visible: root.pluggedIn && root.batteryIsPowerSupply && root.chargeStopThreshold > 0 && root.chargeStopThreshold < 100
                iconSource: "kt-speed-limits" // FIXME good icon
                text: i18n("Battery is configured to charge up to approximately %1%.", root.chargeStopThreshold)
            }
        }
    }
}
