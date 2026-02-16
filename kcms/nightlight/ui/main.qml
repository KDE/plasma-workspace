/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.config as KConfig
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.private.kcms.nightlight as Private

KCM.SimpleKCM {
    id: root

    property bool defaultRequested: false

    implicitHeight: Kirigami.Units.gridUnit * 29
    implicitWidth: Kirigami.Units.gridUnit * 35

    Timer {
        id: previewTimer
        interval: Kirigami.Units.humanMoment
        onTriggered: kcm.stopPreview()
    }

    ColumnLayout {
        spacing: 0

        QQC2.Label {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.gridUnit

            text: i18nc("@info", "The blue light filter makes the colors on the screen warmer.")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Kirigami.FormLayout {
            id: parentLayout

            RowLayout {
                Kirigami.FormData.label: i18nc("@label:listbox Night Light modes", "Switching times:")
                spacing: Kirigami.Units.smallSpacing

                QQC2.ComboBox {
                    id: modeSwitcher
                    // Work around https://bugs.kde.org/show_bug.cgi?id=403153
                    Layout.minimumWidth: Kirigami.Units.gridUnit * 17
                    currentIndex: kcm.nightLightSettings.active ? kcm.nightLightSettings.mode + 1 : 0
                    model: [
                        i18nc("@item:inlistbox switching times", "Always off"),  // This is not actually a Mode, but represents Night Light being disabled
                        i18nc("@item:inlistbox switching times", "Always on night light"),
                        i18nc("@item:inlistbox switching times", "Sunrise and sunset")
                    ]
                    onCurrentIndexChanged: {
                        if (currentIndex !== 0) {
                            kcm.nightLightSettings.mode = currentIndex - 1;
                        }
                        kcm.nightLightSettings.active = (currentIndex !== 0);
                    }
                }

                QQC2.Button {
                    icon.name: "configure"
                    text: i18nc("@action:button Configure day-night cycle times", "Configure…")
                    display: QQC2.AbstractButton.IconOnly

                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

                    enabled: kcm.nightLightSettings.active && kcm.nightLightSettings.mode === Private.NightLightMode.DarkLight && KConfig.KAuthorized.authorizeControlModule("kcm_nighttime")
                    onClicked: KCM.KCMLauncher.open("kcm_nighttime")
                }
            }
        }

        DayNightView {
            Layout.margins: Kirigami.Units.smallSpacing
            Layout.bottomMargin: Kirigami.Units.gridUnit
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            Layout.alignment: Qt.AlignCenter

            enabled: kcm.nightLightSettings.active
            dayTemperature: kcm.nightLightSettings.dayTemperature
            nightTemperature: kcm.nightLightSettings.nightTemperature
            alwaysOn: kcm.nightLightSettings.mode === Private.NightLightMode.Constant
            dayTransitionOn: minutesForDate(dayNightTimings.morningStart)
            dayTransitionOff: minutesForDate(dayNightTimings.morningEnd)
            nightTransitionOn: minutesForDate(dayNightTimings.eveningStart)
            nightTransitionOff: minutesForDate(dayNightTimings.eveningEnd)

            function minutesForFixed(date: date): int {
                return date.getHours() * 60 + date.getMinutes();
            }

            Private.DayNightTimings {
                id: dayNightTimings
                dateTime: new Date()
            }
        }

        QQC2.Label {
            id: sliderValueLabelMetrics
            visible: false
            // One digit more than the theoretical maximum, because who knows
            // which digit is the widest in the current font anyway.
            text: i18nc("Color temperature in Kelvin", "%1 K", 99999)
            textFormat: Text.PlainText
        }

        Kirigami.FormLayout {
            twinFormLayouts: parentLayout

            GridLayout {
                Kirigami.FormData.label: i18nc("@label:slider", "Day light temperature:")
                Kirigami.FormData.buddyFor: tempSliderDay
                enabled: kcm.nightLightSettings.active && kcm.nightLightSettings.mode !== Private.NightLightMode.Constant

                columns: 4

                QQC2.Slider {
                    id: tempSliderDay
                    // Match combobox width
                    Layout.minimumWidth: modeSwitcher.width
                    Layout.columnSpan: 3
                    from: kcm.maxDayTemp
                    to: kcm.minDayTemp
                    stepSize: -100
                    live: true

                    value: kcm.nightLightSettings.dayTemperature

                    onMoved: {
                        kcm.nightLightSettings.dayTemperature = value
                        kcm.preview(value)

                        // This can fire for scroll events; in this case we need
                        // to use a timer to make the preview message disappear, since
                        // we can't make it disappear in the onPressedChanged handler
                        // since there is no press
                        if (!pressed) {
                            previewTimer.restart()
                        }
                    }
                    onPressedChanged: {
                        if (!pressed) {
                            kcm.stopPreview()
                        }
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.nightLightSettings
                        settingName: "DayTemperature"
                        extraEnabledConditions: kcm.nightLightSettings.active
                    }
                }
                QQC2.Label {
                    text: i18nc("Color temperature in Kelvin", "%1 K", tempSliderDay.value)
                    textFormat: Text.PlainText
                    horizontalAlignment: Text.AlignRight
                    Layout.minimumWidth: sliderValueLabelMetrics.implicitWidth
                }
                //row 2
                QQC2.Label {
                    text: i18nc("Night colour blue-ish; no blue light filter activated", "Cool (no filter)")
                    textFormat: Text.PlainText
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: i18nc("Night colour red-ish", "Warm")
                    textFormat: Text.PlainText
                }
                Item {}
            }

            GridLayout {
                Kirigami.FormData.label: i18nc("@label:slider", "Night light temperature:")
                Kirigami.FormData.buddyFor: tempSliderNight
                enabled: kcm.nightLightSettings.active

                columns: 4

                QQC2.Slider {
                    id: tempSliderNight
                    // Match combobox width
                    Layout.minimumWidth: modeSwitcher.width
                    Layout.columnSpan: 3
                    from: kcm.maxNightTemp
                    to: kcm.minNightTemp
                    stepSize: -100
                    live: true

                    value: kcm.nightLightSettings.nightTemperature

                    onMoved: {
                        kcm.nightLightSettings.nightTemperature = value
                        kcm.preview(value)

                        // This can fire for scroll events; in this case we need
                        // to use a timer to make the preview disappear, since
                        // we can't make it disappear in the onPressedChanged handler
                        // since there is no press
                        if (!pressed) {
                            previewTimer.restart()
                        }
                    }
                    onPressedChanged: {
                        if (!pressed) {
                            kcm.stopPreview()
                        }
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.nightLightSettings
                        settingName: "NightTemperature"
                        extraEnabledConditions: kcm.nightLightSettings.active
                    }
                }
                QQC2.Label {
                    text: i18nc("Color temperature in Kelvin", "%1 K", tempSliderNight.value)
                    textFormat: Text.PlainText
                    horizontalAlignment: Text.AlignRight
                    Layout.minimumWidth: sliderValueLabelMetrics.implicitWidth
                }
                //row 2
                QQC2.Label {
                    text: i18nc("Night colour blue-ish; no blue light filter activated", "Cool (no filter)")
                    textFormat: Text.PlainText
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: i18nc("Night colour red-ish", "Warm")
                    textFormat: Text.PlainText
                }
                Item {}
            }
        }
    }
}
