/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtPositioning
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.private.kcms.nighttime as Private

KCM.SimpleKCM {
    id: root

    ColumnLayout {
    QQC2.Label {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.gridUnit
        Layout.maximumWidth: form.implicitWidth
        Layout.alignment: Qt.AlignCenter

        text: i18nc("@info", "Time-based features such as Night Light and dynamic wallpapers can be synchronized with the day-night cycle. Choose your preferred cycle here.")
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }

    Kirigami.Form {
        id: form
        Layout.fillWidth: true
        Kirigami.FormGroup {

            Kirigami.FormEntry {
                fullWidth: true
                contentItem: RowLayout {
                    Layout.fillWidth: true
                    Item {
                        Layout.fillWidth: true
                    }
                    SunPath {
                        Layout.fillWidth: true
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 30
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
                        Layout.alignment: Qt.AlignHCenter
                        schedule: schedulePreview
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
            }

            Kirigami.FormSeparator {}

            Kirigami.FormEntry {
                contentItem: QQC2.ComboBox {
                    id: sourceComboBox
                    Layout.minimumWidth: Kirigami.Units.gridUnit * 17
                    Kirigami.FormData.label: i18nc("@label:listbox part of a sentence: 'Determine sunrise and sunset times [based on]'", "Determine sunrise and sunset times:")
                    currentIndex: kcm.settings.source
                    textRole: "text"
                    valueRole: "value"
                    model: [
                        { value: Private.NightTimeSettings.Location, text: i18nc("@item:inlistbox part of a sentence: 'Determine sunrise and sunset times'", "Based on device's location") },
                        { value: Private.NightTimeSettings.Times, text: i18nc("@item:inlistbox part of a sentence: 'Determine sunrise and sunset times'", "Using custom times") }
                    ]
                    onActivated: kcm.settings.source = currentValue;

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "source"
                    }
                }
            }

            Kirigami.FormEntry {
                visible: kcm.settings.source === Private.NightTimeSettings.Times
                contentItem: TimeField {
                    id: sunriseStartTimeField
                    Kirigami.FormData.label: i18nc("@label", "Sunrise:")
                    value: kcm.settings.sunriseStart
                    onActivated: (value) => {
                        kcm.settings.sunriseStart = Private.DarkLightScheduleValidator.validate(value, kcm.settings.sunsetStart, kcm.settings.transitionDuration);
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "sunriseStart"
                    }
                }
            }

            Kirigami.FormEntry {
                visible: kcm.settings.source === Private.NightTimeSettings.Times
                contentItem: TimeField {
                    id: sunsetStartTimeField
                    Kirigami.FormData.label: i18nc("@label", "Sunset:")
                    value: kcm.settings.sunsetStart
                    onActivated: (value) => {
                        kcm.settings.sunsetStart = Private.DarkLightScheduleValidator.validate(value, kcm.settings.sunriseStart, kcm.settings.transitionDuration);
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "sunsetStart"
                    }
                }
            }

            Kirigami.FormEntry {
                visible: kcm.settings.source === Private.NightTimeSettings.Times
                contentItem: QQC2.SpinBox {
                    Kirigami.FormData.label: i18nc("@label:spinbox", "Transition duration:")
                    from: 60
                    to: 36000
                    stepSize: 300
                    value: kcm.settings.transitionDuration
                    textFromValue: (value, locale) => i18ncp("@item:valuesuffix transition duration", "%1 minute", "%1 minutes", Math.round(value / 60))
                    valueFromText: (text, locale) => parseInt(text) * 60
                    onValueModified: {
                        kcm.settings.sunsetStart = Private.DarkLightScheduleValidator.validate(kcm.settings.sunsetStart, kcm.settings.sunriseStart, value);
                        kcm.settings.transitionDuration = value;
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "transitionDuration"
                    }
                }
            }

            Kirigami.FormEntry {
                fullWidth: true
                visible: kcm.settings.source === Private.NightTimeSettings.Location
                subtitle: automaticLocationProvider.name === "geoclue2"
                        ? xi18nc("@info", "The <application>GeoClue2</application> service will be used to periodically update the device's location using GPS or cell tower triangulation if available, or else by sending its IP address to <link url='https://geoip.com/privacy/'>GeoIP</link>.")
                        : xi18nc("@info", "The <application>%1</application> service will be used to periodically update the device's location. Please open a bug report at <link url='https://bugs.kde.org'>https://bugs.kde.org</link> asking KDE developers to write a detailed description of what this service will do.", automaticLocationProvider.name)
                contentItem: QQC2.RadioButton {
                    QQC2.ButtonGroup.group: locationGroup
                    text: i18nc("@option:radio", "Automatically detect location")
                    checked: kcm.settings.automaticLocation
                    onToggled: kcm.settings.automaticLocation = true;

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "automaticLocation"
                    }
                    QQC2.ButtonGroup { id: locationGroup }
                }
            }

            Kirigami.FormEntry {
                fullWidth: true
                visible: kcm.settings.source === Private.NightTimeSettings.Location
                contentItem: RowLayout {
                    Layout.fillWidth: true
                    Kirigami.FormData.buddyFor: manualLocationButton
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.RadioButton {
                        id: manualLocationButton
                        QQC2.ButtonGroup.group: locationGroup
                        text: i18nc("@option:radio", "Use manual location:")
                        checked: !kcm.settings.automaticLocation
                        onToggled: kcm.settings.automaticLocation = false;

                        KCM.SettingStateBinding {
                            configObject: kcm.settings
                            settingName: "automaticLocation"
                        }
                    }

                    QQC2.Label {
                        enabled: manualLocationButton.checked
                        text: i18nc("@label", "Latitude:")
                        textFormat: Text.PlainText
                    }

                    NumberField {
                        implicitWidth: 4 * Kirigami.Units.gridUnit
                        from: -90
                        to: 90
                        value: kcm.settings.manualLatitude
                        onValueEdited: (value) => kcm.settings.manualLatitude = value;

                        KCM.SettingStateBinding {
                            configObject: kcm.settings
                            settingName: "manualLatitude"
                            extraEnabledConditions: manualLocationButton.checked
                        }
                    }

                    QQC2.Label {
                        enabled: manualLocationButton.checked
                        text: i18nc("@label", "Longitude:")
                        textFormat: Text.PlainText
                    }

                    NumberField {
                        implicitWidth: 4 * Kirigami.Units.gridUnit
                        from: -180
                        to: 180
                        value: kcm.settings.manualLongitude
                        onValueEdited: (value) => kcm.settings.manualLongitude = value;

                        KCM.SettingStateBinding {
                            configObject: kcm.settings
                            settingName: "manualLongitude"
                            extraEnabledConditions: manualLocationButton.checked
                        }
                    }
                }
            }

            Kirigami.FormEntry {
                fullWidth: true
                visible: kcm.settings.source === Private.NightTimeSettings.Location
                contentItem: RowLayout {
                    Layout.fillWidth: true
                    Item {
                        Layout.fillWidth: true
                    }
                    WorldMap {
                        Layout.alignment: Qt.AlignHCenter
                        latitude: if (kcm.settings.automaticLocation) {
                            return automaticLocationProvider.position.coordinate.latitude;
                        } else {
                            return kcm.settings.manualLatitude;
                        }
                        longitude: if (kcm.settings.automaticLocation) {
                            return automaticLocationProvider.position.coordinate.longitude;
                        } else {
                            return kcm.settings.manualLongitude;
                        }
                        onAccepted: (latitude, longitude) => {
                            kcm.settings.manualLatitude = latitude;
                            kcm.settings.manualLongitude = longitude;
                            kcm.settings.automaticLocation = false;
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    PositionSource {
        id: automaticLocationProvider
        active: kcm.settings.source === Private.NightTimeSettings.Location && kcm.settings.automaticLocation
    }

    Private.DarkLightSchedulePreview {
        id: schedulePreview
        coordinate: {
            if (kcm.settings.source === Private.NightTimeSettings.Location) {
                if (kcm.settings.automaticLocation) {
                    return automaticLocationProvider.position.coordinate;
                } else {
                    return QtPositioning.coordinate(kcm.settings.manualLatitude, kcm.settings.manualLongitude);
                }
            } else {
                return undefined;
            }
        }
        sunsetStart: kcm.settings.sunsetStart
        sunriseStart: kcm.settings.sunriseStart
        transitionDuration: kcm.settings.transitionDuration
    }
    }
}
