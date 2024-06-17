/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtPositioning

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

import org.kde.colorcorrect as CC

import org.kde.private.kcms.nightlight as Private

KCM.SimpleKCM {
    id: root

    readonly property int error: compositorAdaptor.error
    property bool defaultRequested: false

    implicitHeight: Kirigami.Units.gridUnit * 29
    implicitWidth: Kirigami.Units.gridUnit * 35

    CC.CompositorAdaptor {
        id: compositorAdaptor
    }

    CC.SunCalc {
        id: sunCalc
    }

    PositionSource {
        id: automaticLocationProvider
        active: kcm.nightLightSettings.active && kcm.nightLightSettings.mode === Private.NightLightMode.Automatic

        readonly property bool locating: automaticLocationProvider.active
            && automaticLocationProvider.sourceError == PositionSource.NoError
            && !(automaticLocationProvider.position.latitudeValid || automaticLocationProvider.position.longitudeValid)

        onPositionChanged: {
            kcm.nightLightSettings.latitudeAuto = Math.round(automaticLocationProvider.position.coordinate.latitude * 100) / 100;
            kcm.nightLightSettings.longitudeAuto = Math.round(automaticLocationProvider.position.coordinate.longitude * 100) / 100;
        }
    }

    headerPaddingEnabled: false // Let the InlineMessage touch the edges
    header: Kirigami.InlineMessage {
        id: errorMessage
        visible: compositorAdaptor.error !== CC.CompositorAdaptor.ErrorCodeSuccess
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Error
        text: compositorAdaptor.errorText
    }

    Timer {
        id: previewTimer
        interval: Kirigami.Units.humanMoment
        onTriggered: compositorAdaptor.stopPreview()
    }

    ColumnLayout {
        spacing: 0

        QQC2.Label {
            Layout.margins: Kirigami.Units.gridUnit
            Layout.alignment: Qt.AlignHCenter

            Layout.maximumWidth: Math.round(root.width - (Kirigami.Units.gridUnit * 2))
            text: i18n("The blue light filter makes the colors on the screen warmer.")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }

        DayNightView {
            Layout.margins: Kirigami.Units.smallSpacing
            Layout.bottomMargin: Kirigami.Units.gridUnit
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            Layout.alignment: Qt.AlignCenter

            readonly property real latitude: kcm.nightLightSettings.mode === Private.NightLightMode.Location
                ? kcm.nightLightSettings.latitudeFixed
                : automaticLocationProvider.position.latitudeValid ? automaticLocationProvider.position.coordinate.latitude : kcm.nightLightSettings.latitudeAuto
            readonly property real longitude: kcm.nightLightSettings.mode === Private.NightLightMode.Location
                ? kcm.nightLightSettings.longitudeFixed
                : automaticLocationProvider.position.longitudeValid ? automaticLocationProvider.position.coordinate.longitude : kcm.nightLightSettings.longitudeAuto

            readonly property var morningTimings: sunCalc.getMorningTimings(latitude, longitude)
            readonly property var eveningTimings: sunCalc.getEveningTimings(latitude, longitude)

            enabled: kcm.nightLightSettings.active

            dayTemperature: kcm.nightLightSettings.dayTemperature
            nightTemperature: kcm.nightLightSettings.nightTemperature

            alwaysOn: kcm.nightLightSettings.mode === Private.NightLightMode.Constant
            dayTransitionOn: kcm.nightLightSettings.mode === Private.NightLightMode.Timings
                ? minutesForFixed(kcm.nightLightSettings.morningBeginFixed)
                : minutesForDate(morningTimings.begin)
            dayTransitionOff: kcm.nightLightSettings.mode === Private.NightLightMode.Timings
                ? (minutesForFixed(kcm.nightLightSettings.morningBeginFixed) + kcm.nightLightSettings.transitionTime) % 1440
                : minutesForDate(morningTimings.end)
            nightTransitionOn: kcm.nightLightSettings.mode === Private.NightLightMode.Timings
                ? minutesForFixed(kcm.nightLightSettings.eveningBeginFixed)
                : minutesForDate(eveningTimings.begin)
            nightTransitionOff: kcm.nightLightSettings.mode === Private.NightLightMode.Timings
                ? (minutesForFixed(kcm.nightLightSettings.eveningBeginFixed) + kcm.nightLightSettings.transitionTime) % 1440
                : minutesForDate(eveningTimings.end)

            function minutesForFixed(dateString: string): int {
                // The fixed timings format is "hhmm"
                const hours = parseInt(dateString.substring(0, 2), 10)
                const mins = parseInt(dateString.substring(2, 4), 10)
                return hours * 60 + mins
            }
        }

        QQC2.Label {
            id: sliderValueLabelMetrics
            visible: false
            // One digit more than the theoretical maximum, because who knows
            // which digit is the widest in the current font anyway.
            text: i18nc("Color temperature in Kelvin", "%1K", 99999)
            textFormat: Text.PlainText
        }

        Kirigami.FormLayout {
            id: parentLayout

            GridLayout {
                Kirigami.FormData.label: i18n("Day light temperature:")
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
                        compositorAdaptor.preview(value)

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
                            compositorAdaptor.stopPreview()
                        }
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.nightLightSettings
                        settingName: "DayTemperature"
                        extraEnabledConditions: kcm.nightLightSettings.active
                    }
                }
                QQC2.Label {
                    text: i18nc("Color temperature in Kelvin", "%1K", tempSliderDay.value)
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
                Kirigami.FormData.label: i18n("Night light temperature:")
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
                        compositorAdaptor.preview(value)

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
                            compositorAdaptor.stopPreview()
                        }
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.nightLightSettings
                        settingName: "NightTemperature"
                        extraEnabledConditions: kcm.nightLightSettings.active
                    }
                }
                QQC2.Label {
                    text: i18nc("Color temperature in Kelvin", "%1K", tempSliderNight.value)
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

            Item { implicitHeight: Kirigami.Units.largeSpacing }

            QQC2.ComboBox {
                id: modeSwitcher
                // Work around https://bugs.kde.org/show_bug.cgi?id=403153
                Layout.minimumWidth: Kirigami.Units.gridUnit * 17
                Kirigami.FormData.label: i18n("Switching times:")
                currentIndex: kcm.nightLightSettings.active ? kcm.nightLightSettings.mode + 1 : 0
                model: [
                    i18n("Always off"),  // This is not actually a Mode, but represents Night Light being disabled
                    i18n("Sunset and sunrise at current location"),
                    i18n("Sunset and sunrise at manual location"),
                    i18n("Custom times"),
                    i18n("Always on night light")
                ]
                onCurrentIndexChanged: {
                    if (currentIndex !== 0) {
                        kcm.nightLightSettings.mode = currentIndex - 1;
                    }
                    kcm.nightLightSettings.active = (currentIndex !== 0);
                }
            }

            // Show current location in auto mode
            QQC2.Label {
                Kirigami.FormData.label: i18nc("@label The coordinates for the current location", "Current location:")

                visible: automaticLocationProvider.active && !automaticLocationProvider.locating
                enabled: kcm.nightLightSettings.active
                wrapMode: Text.Wrap
                text: i18n("Latitude: %1°   Longitude: %2°",
                    Math.round((automaticLocationProvider.position.latitudeValid ? automaticLocationProvider.position.coordinate.latitude : 0) * 100) / 100,
                    Math.round((automaticLocationProvider.position.longitudeValid ? automaticLocationProvider.position.coordinate.longitude : 0) * 100) / 100)
                textFormat: Text.PlainText
            }

            // Inform about geolocation access in auto mode
            // The system settings window likes to take over the cursor with a plain label.
            // The TextEdit 'takes priority' over the system settings window trying to eat the mouse,
            // allowing us to use the HoverHandler boilerplate for proper link handling
            TextEdit {
                Layout.maximumWidth: modeSwitcher.width

                visible: modeSwitcher.currentIndex - 1 === Private.NightLightMode.Automatic && kcm.nightLightSettings.active
                enabled: kcm.nightLightSettings.active

                textFormat: TextEdit.RichText
                wrapMode: Text.Wrap
                readOnly: true

                color: Kirigami.Theme.textColor
                selectedTextColor: Kirigami.Theme.highlightedTextColor
                selectionColor: Kirigami.Theme.highlightColor

                text: xi18nc("@info", "The device's location will be periodically updated using GPS (if available), or by sending network information to <link url='https://location.services.mozilla.com'>Mozilla Location Service</link>.")
                font: Kirigami.Theme.smallFont

                onLinkActivated: (url) => Qt.openUrlExternally(url)

                HoverHandler {
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            // Show time entry fields in manual timings mode
            TimeField {
                id: eveningBeginFixedField
                visible: kcm.nightLightSettings.mode === Private.NightLightMode.Timings && kcm.nightLightSettings.active
                Kirigami.FormData.label: i18n("Begin night light at:")

                backend: kcm.nightLightSettings.eveningBeginFixed
                onBackendChanged: {
                    morningBeginFixedField.preventOverlapWith(backendToDate(), transitionDurationField.value)
                    kcm.nightLightSettings.eveningBeginFixed = backend;
                }
                KCM.SettingStateBinding {
                    configObject: kcm.nightLightSettings
                    settingName: "EveningBeginFixed"
                    extraEnabledConditions: kcm.nightLightSettings.active && kcm.nightLightSettings.mode === Private.NightLightMode.Timings
                }
            }

            TimeField {
                id: morningBeginFixedField
                visible: kcm.nightLightSettings.mode === Private.NightLightMode.Timings && kcm.nightLightSettings.active
                Kirigami.FormData.label: i18n("Begin day light at:")
                backend: kcm.nightLightSettings.morningBeginFixed
                onBackendChanged: {
                    eveningBeginFixedField.preventOverlapWith(backendToDate(), transitionDurationField.value)
                    kcm.nightLightSettings.morningBeginFixed = backend;
                }
                KCM.SettingStateBinding {
                    configObject: kcm.nightLightSettings
                    settingName: "MorningBeginFixed"
                    extraEnabledConditions: kcm.nightLightSettings.active && kcm.nightLightSettings.mode === Private.NightLightMode.Timings
                }
            }

            QQC2.SpinBox {
                id: transitionDurationField
                visible: kcm.nightLightSettings.mode === Private.NightLightMode.Timings && kcm.nightLightSettings.active
                Kirigami.FormData.label: i18n("Transition duration:")
                from: 1
                to: 600 // less than 10 hours (in minutes: 600)
                stepSize: 5
                value: kcm.nightLightSettings.transitionTime
                editable: true
                onValueModified: {
                    kcm.nightLightSettings.transitionTime = value;
                    eveningBeginFixedField.preventOverlapWith(morningBeginFixedField.backendToDate(), value);
                }
                textFromValue: function(value, locale) {
                    return i18np("%1 minute", "%1 minutes", value);
                }
                valueFromText: function(text, locale) {
                    return parseInt(text);
                }

                KCM.SettingStateBinding {
                    configObject: kcm.nightLightSettings
                    settingName: "TransitionTime"
                    extraEnabledConditions: kcm.nightLightSettings.active
                }

                QQC2.ToolTip {
                    text: i18n("Input minutes - min. 1, max. 600")
                }
            }
        }

        // Show location chooser in manual location mode
        LocationsFixedView {
            visible: kcm.nightLightSettings.mode === Private.NightLightMode.Location && kcm.nightLightSettings.active
            Layout.alignment: Qt.AlignHCenter
            enabled: kcm.nightLightSettings.active
        }

        Item {
            visible: automaticLocationProvider.locating
            Layout.topMargin: Kirigami.Units.largeSpacing * 4
            Layout.fillWidth: true
            implicitHeight: loadingPlaceholder.implicitHeight

            Kirigami.LoadingPlaceholder {
                id: loadingPlaceholder

                text: i18nc("@info:placeholder", "Locating…")
                anchors.centerIn: parent
            }
        }
    }
}
