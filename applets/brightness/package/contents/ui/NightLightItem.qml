/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 * SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
 * SPDX-FileCopyrightText: 2023 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts

import org.kde.kcmutils // KCMLauncher
import org.kde.config as KConfig  // KAuthorized.authorizeControlModule
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

import org.kde.plasma.private.brightnesscontrolplugin

PlasmaComponents3.ItemDelegate {
    id: root

    background.visible: highlighted
    highlighted: activeFocus
    hoverEnabled: false

    Accessible.description: status.text

    contentItem: RowLayout {
        spacing: Kirigami.Units.gridUnit

        Kirigami.Icon {
            id: image
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: Kirigami.Units.iconSizes.medium
            source: {
                if (!control.enabled) {
                    return "redshift-status-on"; // not configured: show generic night light icon rather "manually turned off" icon
                } else if (!control.running) {
                    return "redshift-status-off";
                } else if (control.daylight && control.targetTemperature != 6500) { // show daylight icon only when temperature during the day is actually modified
                    return "redshift-status-day";
                } else {
                    return "redshift-status-on";
                }
            }
        }

        KeyNavigation.tab: inhibitionSwitch.visible ? inhibitionSwitch : kcmButton

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.Label {
                    id: title
                    text: root.text
                    textFormat: Text.PlainText

                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                PlasmaComponents3.Label {
                    id: status
                    text: {
                        if (control.inhibited && control.enabled) {
                            return i18nc("Night light status", "Suspended");
                        }
                        if (!control.available) {
                            return i18nc("Night light status", "Unavailable");
                        }
                        if (!control.enabled) {
                            return i18nc("Night light status", "Not enabled");
                        }
                        if (!control.running) {
                            return i18nc("Night light status", "Not running");
                        }
                        if (!control.hasSwitchingTimes) {
                            return i18nc("Night light status", "On");
                        }
                        if (control.daylight && control.transitioning) {
                            return i18nc("Night light phase", "Morning Transition");
                        } else if (control.daylight) {
                            return i18nc("Night light phase", "Day");
                        } else if (control.transitioning) {
                            return i18nc("Night light phase", "Evening Transition");
                        } else {
                            return i18nc("Night light phase", "Night");
                        }
                    }
                    textFormat: Text.PlainText

                    enabled: false
                }

                PlasmaComponents3.Label {
                    id: currentTemp
                    visible: control.available && control.enabled && control.running
                    text: i18nc("Placeholder is screen color temperature", "%1K", control.currentTemperature)
                    textFormat: Text.PlainText

                    horizontalAlignment: Text.AlignRight
                }
            }

            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.Switch {
                    id: inhibitionSwitch
                    visible: control.enabled
                    enabled: control.togglable
                    checked: control.inhibited
                    text: i18nc("@action:button Night Light", "Suspend")

                    Layout.fillWidth: true

                    Accessible.onPressAction: toggled()

                    KeyNavigation.up: root.KeyNavigation.up
                    KeyNavigation.tab: kcmButton
                    KeyNavigation.right: kcmButton
                    KeyNavigation.backtab: root

                    Keys.onPressed: (event) => {
                        if (event.key == Qt.Key_Space || event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                            toggle();
                        }
                    }
                    onToggled: control.toggleInhibition()
                }

                PlasmaComponents3.Button {
                    id: kcmButton
                    visible: KConfig.KAuthorized.authorizeControlModule("kcm_nightlight")

                    icon.name: "configure"
                    text: control.enabled ? i18n("Configure…") : i18n("Enable and Configure…")

                    Layout.alignment: Qt.AlignRight

                    KeyNavigation.up: root.KeyNavigation.up
                    KeyNavigation.backtab: inhibitionSwitch.visible ? inhibitionSwitch : root
                    KeyNavigation.left: inhibitionSwitch

                    Keys.onPressed: (event) => {
                        if (event.key == Qt.Key_Space || event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                            clicked();
                        }
                    }
                    onClicked: KCMLauncher.openSystemSettings("kcm_nightlight")
                }
            }

            RowLayout {
                visible: control.running && control.hasSwitchingTimes

                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.Label {
                    id: transitionLabel
                    text: {
                        if (control.daylight && control.transitioning) {
                            return i18nc("Label for a time", "Transition to day complete by:");
                        } else if (control.daylight) {
                            return i18nc("Label for a time", "Transition to night scheduled for:");
                        } else if (control.transitioning) {
                            return i18nc("Label for a time", "Transition to night complete by:");
                        } else {
                            return i18nc("Label for a time", "Transition to day scheduled for:");
                        }
                    }
                    textFormat: Text.PlainText

                    enabled: false
                    font: Kirigami.Theme.smallFont
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                PlasmaComponents3.Label {
                    id: transitionTime
                    text: {
                        if (control.transitioning) {
                            return new Date(control.currentTransitionEndTime).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
                        } else {
                            return new Date(control.scheduledTransitionStartTime).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
                        }
                    }
                    textFormat: Text.PlainText

                    enabled: false
                    font: Kirigami.Theme.smallFont
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignRight
                }
            }

        }
    }

    NightLightControl {
        id: control

        readonly property bool transitioning: control.currentTemperature != control.targetTemperature
        readonly property bool hasSwitchingTimes: control.mode != 3
        readonly property bool togglable: !control.inhibited || control.inhibitedFromApplet
    }

}
