/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.private.kcms.nighttime as Private

KCM.SimpleKCM {
    id: root

    ColumnLayout {
        spacing: 0

        Label {
            Layout.margins: Kirigami.Units.gridUnit
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: root.width - Kirigami.Units.gridUnit * 2

            text: i18n("The dark-light cycle describes the rhythm of day and night. Features such as Night Light and dynamic wallpapers can be synchronized with it.")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        ColumnLayout {
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            Layout.alignment: Qt.AlignHCenter

            spacing: Kirigami.Units.gridUnit * 0.5

            Private.SunPathChart {
                id: sunPathChart
                Layout.fillWidth: true
                Layout.minimumHeight: Kirigami.Units.gridUnit * 5
                Layout.alignment: Qt.AlignHCenter
                sunriseDateTime: schedulePreview.endSunriseDateTime
                sunsetDateTime: schedulePreview.startSunsetDateTime
                dayColor: Kirigami.Theme.neutralTextColor
                nightColor: Kirigami.Theme.disabledTextColor
            }

            RowLayout {
                Layout.preferredWidth: sunPathChart.width

                ColumnLayout {
                    Layout.alignment: Qt.AlignLeft
                    Layout.leftMargin: Math.max(Math.round(0.5 * (sunPathChart.width - width) - sunPathChart.daylightSpan), Kirigami.Units.gridUnit)
                    ToolTip.visible: sunriseHoverHandler.hovered
                    ToolTip.text: i18n("Sunrise starts at %1 and ends at %2", formatTime(schedulePreview.startSunriseDateTime), formatTime(schedulePreview.endSunriseDateTime))

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: i18n("Sunrise")
                        textFormat: Text.PlainText
                        font.bold: true
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: formatTime(schedulePreview.endSunriseDateTime)
                        textFormat: Text.PlainText
                    }

                    HoverHandler {
                        id: sunriseHoverHandler
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: Math.max(Math.round(0.5 * (sunPathChart.width - width) - sunPathChart.daylightSpan), Kirigami.Units.gridUnit)
                    ToolTip.visible: sunsetHoverHandler.hovered
                    ToolTip.text: i18n("Sunset starts at %1 and ends at %2", formatTime(schedulePreview.startSunsetDateTime), formatTime(schedulePreview.endSunsetDateTime))

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: i18n("Sunset")
                        textFormat: Text.PlainText
                        font.bold: true
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: formatTime(schedulePreview.startSunsetDateTime)
                        textFormat: Text.PlainText
                    }

                    HoverHandler {
                        id: sunsetHoverHandler
                    }
                }
            }
        }

        Kirigami.FormLayout {
            ComboBox {
                id: sourceComboBox
                Layout.minimumWidth: Kirigami.Units.gridUnit * 17
                Kirigami.FormData.label: i18nc("part of a sentence: 'Determine sunrise and sunset times [based on]'", "Determine sunrise and sunset times:")
                currentIndex: kcm.settings.source
                textRole: "text"
                valueRole: "value"
                model: [
                    { value: Private.NightTimeSettings.AutomaticLocation, text: i18nc("part of a sentence: 'Determine sunrise and sunset times'", "Based on device's current location") },
                    { value: Private.NightTimeSettings.ManualLocation, text: i18nc("part of a sentence: 'Determine sunrise and sunset times'", "Based on manual location") },
                    { value: Private.NightTimeSettings.ManualTimes, text: i18nc("part of a sentence: 'Determine sunrise and sunset times'", "Using custom times") }
                ]
                onActivated: kcm.settings.source = currentValue;

                KCM.SettingStateBinding {
                    configObject: kcm.settings
                    settingName: "Source"
                }
            }

            Label {
                Kirigami.FormData.label: i18nc("@label The coordinates for the current location", "Current location:")
                visible: kcm.settings.source === Private.NightTimeSettings.AutomaticLocation
                wrapMode: Text.Wrap
                text: i18n("Latitude: %1°   Longitude: %2°",
                    Math.round((automaticLocationProvider.position.latitudeValid ? automaticLocationProvider.position.coordinate.latitude : 0) * 100) / 100,
                    Math.round((automaticLocationProvider.position.longitudeValid ? automaticLocationProvider.position.coordinate.longitude : 0) * 100) / 100)
                textFormat: Text.PlainText
            }

            TextEdit {
                Layout.maximumWidth: sourceComboBox.width
                visible: kcm.settings.source === Private.NightTimeSettings.AutomaticLocation
                wrapMode: Text.Wrap
                readOnly: true
                color: Kirigami.Theme.textColor
                font: Kirigami.Theme.smallFont
                selectedTextColor: Kirigami.Theme.highlightedTextColor
                selectionColor: Kirigami.Theme.highlightColor
                textFormat: TextEdit.RichText
                text: automaticLocationProvider.name === "geoclue2"
                    ? xi18nc("@info", "The <application>GeoClue2</application> service will be used to periodically update the device's location using GPS or cell tower triangulation if available, or else by sending its IP address to <link url='https://geoip.com/privacy/'>GeoIP</link>.")
                    : xi18nc("@info", "The <application>%1</application> service will be used to periodically update the device's location. Please open a bug report at <link url='https://bugs.kde.org'>https://bugs.kde.org</link> asking KDE developers to write a detailed description of what this service will do.", automaticLocationProvider.name)

                onLinkActivated: (url) => Qt.openUrlExternally(url)

                HoverHandler {
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            TimeField {
                id: sunriseStartTimeField
                Kirigami.FormData.label: i18n("Sunrise starts at:")
                value: kcm.settings.sunriseStart
                visible: kcm.settings.source === Private.NightTimeSettings.ManualTimes
                onActivated: (value) => {
                    kcm.settings.sunriseStart = Private.DarkLightScheduleValidator.validate(value, kcm.settings.sunsetStart, kcm.settings.transitionDuration);
                }

                KCM.SettingStateBinding {
                    configObject: kcm.settings
                    settingName: "SunriseStart"
                }
            }

            TimeField {
                id: sunsetStartTimeField
                Kirigami.FormData.label: i18n("Sunset starts at:")
                value: kcm.settings.sunsetStart
                visible: kcm.settings.source === Private.NightTimeSettings.ManualTimes
                onActivated: (value) => {
                    kcm.settings.sunsetStart = Private.DarkLightScheduleValidator.validate(value, kcm.settings.sunriseStart, kcm.settings.transitionDuration);
                }

                KCM.SettingStateBinding {
                    configObject: kcm.settings
                    settingName: "SunsetStart"
                }
            }

            SpinBox {
                Kirigami.FormData.label: i18n("Transition duration:")
                visible: kcm.settings.source === Private.NightTimeSettings.ManualTimes
                from: 60
                to: 36000
                stepSize: 300
                value: kcm.settings.transitionDuration
                textFromValue: (value, locale) => i18np("%1 minute", "%1 minutes", Math.round(value / 60))
                valueFromText: (text, locale) => parseInt(text) * 60
                onValueModified: {
                    kcm.settings.sunsetStart = Private.DarkLightScheduleValidator.validate(value, kcm.settings.sunriseStart, value * 60);
                    kcm.settings.transitionDuration = value * 60;
                }

                KCM.SettingStateBinding {
                    configObject: kcm.settings
                    settingName: "TransitionDuration"
                }
            }
        }

        ColumnLayout {
            id: manualLocationOptions
            visible: kcm.settings.source === Private.NightTimeSettings.ManualLocation

            WorldMap {
                latitude: kcm.settings.latitude
                longitude: kcm.settings.longitude
                onAccepted: (latitude, longitude) => {
                    kcm.settings.latitude = latitude;
                    kcm.settings.longitude = longitude;
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter

                Label {
                    text: i18n("Latitude:")
                    textFormat: Text.PlainText
                }

                NumberField {
                    from: -90
                    to: 90
                    value: kcm.settings.latitude
                    onValueEdited: (value) => kcm.settings.latitude = value;

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "Latitude"
                        extraEnabledConditions: manualLocationOptions.visible
                    }
                }

                Label {
                    text: i18n("Longitude:")
                    textFormat: Text.PlainText
                }

                NumberField {
                    from: -180
                    to: 180
                    value: kcm.settings.longitude
                    onValueEdited: (value) => kcm.settings.longitude = value;

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "Longitude"
                        extraEnabledConditions: manualLocationOptions.visible
                    }
                }
            }
        }
    }

    PositionSource {
        id: automaticLocationProvider
        active: kcm.settings.source === Private.NightTimeSettings.AutomaticLocation
    }

    Private.DarkLightSchedulePreview {
        id: schedulePreview
        coordinate: {
            if (kcm.settings.source === Private.NightTimeSettings.AutomaticLocation) {
                return automaticLocationProvider.position.coordinate;
            } else if (kcm.settings.source === Private.NightTimeSettings.ManualLocation) {
                return QtPositioning.coordinate(kcm.settings.latitude, kcm.settings.longitude);
            } else {
                return undefined;
            }
        }
        sunsetStart: kcm.settings.sunsetStart
        sunriseStart: kcm.settings.sunriseStart
        transitionDuration: kcm.settings.transitionDuration
    }

    function formatTime(dateTime: date): string {
        return dateTime.toLocaleString(Qt.locale(), "h:mm");
    }
}
