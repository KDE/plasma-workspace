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
                if (!monitor.enabled) {
                    return "redshift-status-on"; // not configured: show generic night light icon rather "manually turned off" icon
                } else if (!monitor.running) {
                    return "redshift-status-off";
                } else if (monitor.daylight && monitor.targetTemperature != 6500) { // show daylight icon only when temperature during the day is actually modified
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
                        if (inhibitor.state !== NightLightInhibitor.Uninhibited && monitor.enabled) {
                            return i18nc("Night light status", "Off");
                        }
                        if (!monitor.available) {
                            return i18nc("Night light status", "Unavailable");
                        }
                        if (!monitor.enabled) {
                            return i18nc("Night light status", "Not enabled");
                        }
                        if (!monitor.running) {
                            return i18nc("Night light status", "Not running");
                        }
                        if (!monitor.hasSwitchingTimes) {
                            return i18nc("Night light status", "On");
                        }
                        if (monitor.daylight && monitor.transitioning) {
                            return i18nc("Night light phase", "Morning Transition");
                        } else if (monitor.daylight) {
                            return i18nc("Night light phase", "Day");
                        } else if (monitor.transitioning) {
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
                    visible: monitor.available && monitor.enabled && monitor.running && inhibitor.state !== NightLightInhibitor.Inhibited
                    text: i18nc("Placeholder is screen color temperature", "%1K", monitor.currentTemperature)
                    textFormat: Text.PlainText

                    horizontalAlignment: Text.AlignRight
                }
            }

            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.Switch {
                    id: inhibitionSwitch
                    visible: monitor.enabled
                    checked: monitor.available && monitor.enabled && monitor.running && inhibitor.state !== NightLightInhibitor.Inhibited

                    Layout.fillWidth: true

                    KeyNavigation.up: root.KeyNavigation.up
                    KeyNavigation.tab: kcmButton
                    KeyNavigation.right: kcmButton
                    KeyNavigation.backtab: root

                    Keys.onPressed: (event) => {
                        if (event.key == Qt.Key_Space || event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                            toggle();
                        }
                    }
                    onToggled: toggleInhibition()
                }

                PlasmaComponents3.Button {
                    id: kcmButton
                    visible: KConfig.KAuthorized.authorizeControlModule("kcm_nightcolor")

                    icon.name: "configure"
                    text: monitor.enabled ? i18n("Configure…") : i18n("Enable and Configure…")

                    Layout.alignment: Qt.AlignRight

                    KeyNavigation.up: root.KeyNavigation.up
                    KeyNavigation.backtab: inhibitionSwitch.visible ? inhibitionSwitch : root
                    KeyNavigation.left: inhibitionSwitch

                    Keys.onPressed: (event) => {
                        if (event.key == Qt.Key_Space || event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                            clicked();
                        }
                    }
                    onClicked: KCMLauncher.openSystemSettings("kcm_nightcolor")
                }
            }

            RowLayout {
                visible: monitor.running && monitor.hasSwitchingTimes

                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.Label {
                    id: transitionLabel
                    text: {
                        if (monitor.daylight && monitor.transitioning) {
                            return i18nc("Label for a time", "Transition to day complete by:");
                        } else if (monitor.daylight) {
                            return i18nc("Label for a time", "Transition to night scheduled for:");
                        } else if (monitor.transitioning) {
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
                        if (monitor.transitioning) {
                            return new Date(monitor.currentTransitionEndTime).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
                        } else {
                            return new Date(monitor.scheduledTransitionStartTime).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
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

    function toggleInhibition() {
        if (!monitor.available) {
            return;
        }
        switch (inhibitor.state) {
        case NightLightInhibitor.Inhibiting:
        case NightLightInhibitor.Inhibited:
            inhibitor.uninhibit();
            break;
        case NightLightInhibitor.Uninhibiting:
        case NightLightInhibitor.Uninhibited:
            inhibitor.inhibit();
            break;
        }
    }

    NightLightInhibitor {
        id: inhibitor
    }

    NightLightMonitor {
        id: monitor

        readonly property bool transitioning: monitor.currentTemperature != monitor.targetTemperature
        readonly property bool hasSwitchingTimes: monitor.mode != 3
    }

}
