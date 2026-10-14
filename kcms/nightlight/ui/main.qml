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
import org.kde.plasma.workspace.dbus as DBus

KCM.SimpleKCM {
    id: root

    property bool defaultRequested: false

    implicitHeight: Kirigami.Units.gridUnit * 29
    implicitWidth: Kirigami.Units.gridUnit * 35

    DBus.Properties {
        id: dbusInterface
        busType: DBus.BusType.Session
        service: "org.kde.KWin.NightLight"
        path: "/org/kde/KWin/NightLight"
        iface: "org.kde.KWin.NightLight"

        readonly property double activatedUntil: Number(properties.activatedUntil) * 1000
        readonly property bool activatedManual: Boolean(properties.activatedManual)
        readonly property double deactivatedUntil: Number(properties.deactivatedUntil) * 1000
        readonly property bool deactivatedManual: Boolean(properties.deactivatedManual)

        readonly property bool temporarilyActivated: activatedUntil || activatedManual
        readonly property bool temporarilyDeactivated: deactivatedUntil || deactivatedManual

        function activateUntil(dateTime: date): void {
            const timestamp = dateTime ? dateTime.getTime() / 1000 : 0;

            DBus.SessionBus.asyncCall({
                service: "org.kde.KWin.NightLight",
                path: "/org/kde/KWin/NightLight",
                iface: "org.kde.KWin.NightLight",
                member: "activateUntil",
                arguments: [timestamp],
                signature: "(t)",
            });
        }

        function activateManual(on: bool): void {
            DBus.SessionBus.asyncCall({
                service: "org.kde.KWin.NightLight",
                path: "/org/kde/KWin/NightLight",
                iface: "org.kde.KWin.NightLight",
                member: "activateManual",
                arguments: [on],
                signature: "(b)",
            });
        }

        function deactivateUntil(dateTime: date): void {
            const timestamp = dateTime ? dateTime.getTime() / 1000 : 0;

            DBus.SessionBus.asyncCall({
                service: "org.kde.KWin.NightLight",
                path: "/org/kde/KWin/NightLight",
                iface: "org.kde.KWin.NightLight",
                member: "deactivateUntil",
                arguments: [timestamp],
                signature: "(t)",
            });
        }

        function deactivateManual(off: bool): void {
            DBus.SessionBus.asyncCall({
                service: "org.kde.KWin.NightLight",
                path: "/org/kde/KWin/NightLight",
                iface: "org.kde.KWin.NightLight",
                member: "deactivateManual",
                arguments: [off],
                signature: "(b)",
            });
        }
    }

    Timer {
        id: previewTimer
        interval: Kirigami.Units.humanMoment
        onTriggered: kcm.stopPreview()
    }

    ColumnLayout {
        QQC2.Label {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.gridUnit
            Layout.maximumWidth: form.implicitWidth
            Layout.alignment: Qt.AlignCenter

            text: i18nc("@info", "The blue light filter makes the colors on the screen warmer.")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Kirigami.InlineMessage {
            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: form.implicitWidth
            type: Kirigami.MessageType.Information
            visible: dbusInterface.temporarilyActivated || dbusInterface.temporarilyDeactivated
            text: {
                if (dbusInterface.activatedUntil) {
                    return i18nc("@info Placeholder is for time", "Night Light is temporarily activated until %1.", new Date(dbusInterface.activatedUntil).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" }));
                } else if (dbusInterface.activatedManual) {
                    return i18nc("@info", "Night Light is temporarily activated until it is manually turned off.");
                } else if (dbusInterface.deactivatedUntil) {
                    return i18nc("@info Placeholder is for time", "Night Light is temporarily suspended until %1.", new Date(dbusInterface.deactivatedUntil).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" }));
                } else if (dbusInterface.deactivatedManual) {
                    return i18nc("@info", "Night Light is temporarily suspended until it is manually resumed.");
                }

                return "";
            }
            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button turn off Night Light feature", "Turn Off")
                    visible: dbusInterface.temporarilyActivated
                    onTriggered: {
                        if (dbusInterface.activatedManual) {
                            dbusInterface.activateManual(false);
                        } else if (dbusInterface.activatedUntil) {
                            dbusInterface.activateUntil(null);
                        }
                    }
                },
                Kirigami.Action {
                    text: i18nc("@action:button resume Light Light feature, which had been temporarily disabled", "Resume")
                    visible: dbusInterface.temporarilyDeactivated
                    onTriggered: {
                        if (dbusInterface.deactivatedManual) {
                            dbusInterface.deactivateManual(false);
                        } else if (dbusInterface.deactivatedUntil) {
                            dbusInterface.deactivateUntil(null);
                        }
                    }
                },
            ]
        }

        Kirigami.Form {
            id: form
            Layout.fillWidth: true
            Kirigami.FormGroup {
                Kirigami.FormEntry {
                    title: i18nc("@label:listbox Night Light modes", "Switching times:")
                    contentItem: RowLayout {
                        Kirigami.FormData.buddyFor: modeSwitcher
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.ComboBox {
                            id: modeSwitcher
                            // Work around https://bugs.kde.org/show_bug.cgi?id=403153
                            Layout.minimumWidth: Kirigami.Units.gridUnit * 17
                            currentIndex: kcm.nightLightSettings.active ? kcm.nightLightSettings.mode + 1 : 0
                            model: [
                                i18nc("@item:inlistbox switching times", "Always off"),  // This is not actually a Mode, but represents Night Light being disabled
                                i18nc("@item:inlistbox switching times", "Always on"),
                                i18nc("@item:inlistbox switching times", "Sunrise and sunset")
                            ]
                            onCurrentIndexChanged: {
                                if (currentIndex !== 0) {
                                    kcm.nightLightSettings.mode = currentIndex - 1;
                                }
                                kcm.nightLightSettings.active = (currentIndex !== 0);
                            }

                            KCM.SettingStateBinding {
                                configObject: kcm.nightLightSettings
                                settingName: "Active"
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
                        Item {
                            Layout.fillWidth: true
                        }
                    }
                }

                Kirigami.FormSeparator {}

                Kirigami.FormEntry {
                    fullWidth: true
                    contentItem: DayNightView {
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
                }

                QQC2.Label {
                    id: sliderValueLabelMetrics
                    visible: false
                    // One digit more than the theoretical maximum, because who knows
                    // which digit is the widest in the current font anyway.
                    text: i18nc("Color temperature in Kelvin", "%1 K", 99999)
                    textFormat: Text.PlainText
                }

                Kirigami.FormSeparator {}

                Kirigami.FormEntry {
                    title: i18nc("@label:slider", "Day light temperature:")
                    contentItem: GridLayout {
                        Kirigami.FormData.buddyFor: tempSliderDay
                        enabled: (kcm.nightLightSettings.active && kcm.nightLightSettings.mode !== Private.NightLightMode.Constant) || dbusInterface.temporarilyDeactivated

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
                                extraEnabledConditions: kcm.nightLightSettings.active || dbusInterface.temporarilyDeactivated
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
                }

                Kirigami.FormEntry {
                    title: i18nc("@label:slider", "Night light temperature:")
                    contentItem: GridLayout {
                        Kirigami.FormData.buddyFor: tempSliderNight
                        enabled: kcm.nightLightSettings.active || dbusInterface.temporarilyActivated

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
                                extraEnabledConditions: kcm.nightLightSettings.active || dbusInterface.temporarilyActivated
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
    }
}
