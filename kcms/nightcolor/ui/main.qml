/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.20 as Kirigami
import org.kde.kcmutils as KCM

import org.kde.colorcorrect as CC

import org.kde.private.kcms.nightcolor 1.0

KCM.SimpleKCM {
    id: root
    property int error: cA.error
    property bool defaultRequested: false
    property QtObject locator
    readonly property bool doneLocating: locator && !(locator.latitude == 0 && locator.longitude == 0)
    implicitHeight: Kirigami.Units.gridUnit * 29
    implicitWidth: Kirigami.Units.gridUnit * 35

    CC.CompositorAdaptor {
        id: cA
    }

    CC.SunCalc {
        id: sunCalc
    }

    // the Geolocator object is created dynamically so we can have control over when geolocation is attempted
    // because the object attempts geolocation immediately when created, which is unnecessary (and bad for privacy)

    function startLocator() {
        root.locator = Qt.createQmlObject('import org.kde.colorcorrect as CC; CC.Geolocator {}', root, "geoLocatorObj");
    }

    function endLocator() {
        root.locator?.destroy();
    }

    Connections {
        target: kcm.nightColorSettings
        function onActiveChanged() {
            if (kcm.nightColorSettings.active && kcm.nightColorSettings.mode == NightColorMode.Automatic) {
                startLocator();
            } else {
                endLocator();
            }
        }
    }

    Component.onCompleted: {
        if (kcm.nightColorSettings.mode == NightColorMode.Automatic && kcm.nightColorSettings.active) {
            startLocator();
        }
    }

    // Update backend when locator is changed
    Connections {
        target: root.locator
        function onLatitudeChanged() {
            kcm.nightColorSettings.latitudeAuto = Math.round(root.locator.latitude * 100) / 100
        }
        function onLongitudeChanged() {
            kcm.nightColorSettings.longitudeAuto = Math.round(root.locator.longitude * 100) / 100
        }
    }

    header: ColumnLayout{
        Kirigami.InlineMessage {
            id: errorMessage
            Layout.fillWidth: true
            visible: error != CC.CompositorAdaptor.ErrorCodeSuccess
            type: Kirigami.MessageType.Error
            text: cA.errorText
        }
    }

    Timer {
        id: previewTimer
        interval: Kirigami.Units.humanMoment
        onTriggered: cA.stopPreview()
    }

    ColumnLayout {
        spacing: 0

        QQC2.Label {
            Layout.margins: Kirigami.Units.gridUnit
            Layout.alignment: Qt.AlignHCenter

            Layout.maximumWidth: Math.round(root.width - (Kirigami.Units.gridUnit * 2))
            text: i18n("The blue light filter makes the colors on the screen warmer.")
            wrapMode: Text.WordWrap
        }

        DayNightView {
            Layout.margins: Kirigami.Units.smallSpacing
            Layout.bottomMargin: Kirigami.Units.gridUnit
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            Layout.alignment: Qt.AlignCenter

            readonly property real latitude: kcm.nightColorSettings.mode === NightColorMode.Location
                ? kcm.nightColorSettings.latitudeFixed
                : (locator?.locatingDone) ? locator.latitude : kcm.nightColorSettings.latitudeAuto
            readonly property real longitude: kcm.nightColorSettings.mode === NightColorMode.Location
                ? kcm.nightColorSettings.longitudeFixed
                : (locator?.locatingDone) ? locator.longitude : kcm.nightColorSettings.longitudeAuto

            readonly property var morningTimings: sunCalc.getMorningTimings(latitude, longitude)
            readonly property var eveningTimings: sunCalc.getEveningTimings(latitude, longitude)

            enabled: kcm.nightColorSettings.active

            dayTemperature: kcm.nightColorSettings.dayTemperature
            nightTemperature: kcm.nightColorSettings.nightTemperature

            alwaysOn: kcm.nightColorSettings.mode === NightColorMode.Constant
            dayTransitionOn: kcm.nightColorSettings.mode === NightColorMode.Timings
                ? minutesForFixed(kcm.nightColorSettings.morningBeginFixed)
                : minutesForDate(morningTimings.begin)
            dayTransitionOff: kcm.nightColorSettings.mode === NightColorMode.Timings
                ? (minutesForFixed(kcm.nightColorSettings.morningBeginFixed) + kcm.nightColorSettings.transitionTime) % 1440
                : minutesForDate(morningTimings.end)
            nightTransitionOn: kcm.nightColorSettings.mode === NightColorMode.Timings
                ? minutesForFixed(kcm.nightColorSettings.eveningBeginFixed)
                : minutesForDate(eveningTimings.begin)
            nightTransitionOff: kcm.nightColorSettings.mode === NightColorMode.Timings
                ? (minutesForFixed(kcm.nightColorSettings.eveningBeginFixed) + kcm.nightColorSettings.transitionTime) % 1440
                : minutesForDate(eveningTimings.end)

            function minutesForFixed(dateString) {
                // The fixed timings format is "hhmm"
                const hours = parseInt(dateString.substring(0, 2), 10)
                const mins = parseInt(dateString.substring(2, 4), 10)
                return hours * 60 + mins
            }
        }


        Kirigami.FormLayout {
            id: parentLayout

            QQC2.ComboBox {
                id: modeSwitcher
                // Work around https://bugs.kde.org/show_bug.cgi?id=403153
                Layout.minimumWidth: Kirigami.Units.gridUnit * 17
                Kirigami.FormData.label: i18n("Switching times:")
                currentIndex: kcm.nightColorSettings.active ? kcm.nightColorSettings.mode + 1 : 0
                model: [
                    i18n("Always off"),  // This is not actually a Mode, but represents Night Color being disabled
                    i18n("Sunset and sunrise at current location"),
                    i18n("Sunset and sunrise at manual location"),
                    i18n("Custom times"),
                    i18n("Always on night light")
                ]
                onCurrentIndexChanged: {
                    if (currentIndex !== 0) {
                        kcm.nightColorSettings.mode = currentIndex - 1;
                    }
                    kcm.nightColorSettings.active = (currentIndex !== 0);
                    if (currentIndex - 1 == NightColorMode.Automatic && kcm.nightColorSettings.active) {
                        startLocator();
                    } else {
                        endLocator();
                    }
                }
            }

            // Workaround for Layout.margins not working in Kirigami FormLayout (bug 434625)
            Item { implicitHeight: Kirigami.Units.largeSpacing }

            Item {
                Kirigami.FormData.isSection: true
            }

            GridLayout {
                Kirigami.FormData.label: i18n("Day light temperature:")
                Kirigami.FormData.buddyFor: tempSliderDay
                enabled: kcm.nightColorSettings.active && kcm.nightColorSettings.mode !== NightColorMode.Constant

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

                    value: kcm.nightColorSettings.dayTemperature

                    onMoved: {
                        kcm.nightColorSettings.dayTemperature = value
                        cA.preview(value)

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
                            cA.stopPreview()
                        }
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.nightColorSettings
                        settingName: "DayTemperature"
                        extraEnabledConditions: kcm.nightColorSettings.active
                    }
                }
                QQC2.Label {
                    text: i18nc("Color temperature in Kelvin", "%1K", tempSliderDay.value)
                }
                //row 2
                QQC2.Label {
                    text: i18nc("Night colour blue-ish; no blue light filter activated", "Cool (no filter)")
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: i18nc("Night colour red-ish", "Warm")
                }
                Item {}
            }

            GridLayout {
                Kirigami.FormData.label: i18n("Night light temperature:")
                Kirigami.FormData.buddyFor: tempSliderNight
                enabled: kcm.nightColorSettings.active

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

                    value: kcm.nightColorSettings.nightTemperature

                    onMoved: {
                        kcm.nightColorSettings.nightTemperature = value
                        cA.preview(value)

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
                            cA.stopPreview()
                        }
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.nightColorSettings
                        settingName: "NightTemperature"
                        extraEnabledConditions: kcm.nightColorSettings.active
                    }
                }
                QQC2.Label {
                    text: i18nc("Color temperature in Kelvin", "%1K", tempSliderNight.value)
                }
                //row 2
                QQC2.Label {
                    text: i18nc("Night colour blue-ish; no blue light filter activated", "Cool (no filter)")
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: i18nc("Night colour red-ish", "Warm")
                }
                Item {}
            }

            Item { implicitHeight: Kirigami.Units.largeSpacing }

            // Show current location in auto mode
            QQC2.Label {
                Kirigami.FormData.label: i18nc("@label The coordinates for the current location", "Current location:")

                visible: kcm.nightColorSettings.mode === NightColorMode.Automatic && kcm.nightColorSettings.active
                    && root.doneLocating
                enabled: kcm.nightColorSettings.active
                wrapMode: Text.Wrap
                text: i18n("Latitude: %1°   Longitude: %2°", Math.round((locator?.latitude || 0) * 100)/100, Math.round((locator?.longitude || 0) * 100)/100)
            }

            // Inform about geolocation access in auto mode
            // The system settings window likes to take over the cursor with a plain label.
            // The TextEdit 'takes priority' over the system settings window trying to eat the mouse,
            // allowing us to use the HoverHandler boilerplate for proper link handling
            TextEdit {
                Layout.maximumWidth: modeSwitcher.width

                visible: modeSwitcher.currentIndex - 1 === NightColorMode.Automatic && kcm.nightColorSettings.active
                enabled: kcm.nightColorSettings.active

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
                id: evenBeginManField
                // Match combobox width
                Layout.minimumWidth: modeSwitcher.width
                Layout.maximumWidth: modeSwitcher.width
                visible: kcm.nightColorSettings.mode === NightColorMode.Timings && kcm.nightColorSettings.active
                Kirigami.FormData.label: i18n("Begin night light at:")
                backend: kcm.nightColorSettings.eveningBeginFixed
                onBackendChanged: {
                    kcm.nightColorSettings.eveningBeginFixed = backend;
                }

                KCM.SettingStateBinding {
                    configObject: kcm.nightColorSettings
                    settingName: "EveningBeginFixed"
                    extraEnabledConditions: kcm.nightColorSettings.active && kcm.nightColorSettings.mode === NightColorMode.Timings
                }

                QQC2.ToolTip {
                    text: i18n("Input format: HH:MM")
                }
            }

            TimeField {
                id: mornBeginManField
                // Match combobox width
                Layout.minimumWidth: modeSwitcher.width
                Layout.maximumWidth: modeSwitcher.width
                visible: kcm.nightColorSettings.mode === NightColorMode.Timings && kcm.nightColorSettings.active
                Kirigami.FormData.label: i18n("Begin day light at:")
                backend: kcm.nightColorSettings.morningBeginFixed
                onBackendChanged: {
                    kcm.nightColorSettings.morningBeginFixed = backend;
                }

                KCM.SettingStateBinding {
                    configObject: kcm.nightColorSettings
                    settingName: "MorningBeginFixed"
                    extraEnabledConditions: kcm.nightColorSettings.active && kcm.nightColorSettings.mode === NightColorMode.Timings
                }

                QQC2.ToolTip {
                    text: i18n("Input format: HH:MM")
                }
            }

            QQC2.SpinBox {
                id: transTimeField
                visible: kcm.nightColorSettings.mode === NightColorMode.Timings && kcm.nightColorSettings.active
                // Match width of combobox and input fields
                Layout.minimumWidth: modeSwitcher.width
                Kirigami.FormData.label: i18n("Transition duration:")
                from: 1
                to: 600 // less than 12 hours (in minutes: 720)
                value: kcm.nightColorSettings.transitionTime
                editable: true
                onValueModified: {
                    kcm.nightColorSettings.transitionTime = value;
                }
                textFromValue: function(value, locale) {
                    return i18np("%1 minute", "%1 minutes", value)
                }
                valueFromText: function(text, locale) {
                    return parseInt(text);
                }

                KCM.SettingStateBinding {
                    configObject: kcm.nightColorSettings
                    settingName: "TransitionTime"
                    extraEnabledConditions: kcm.nightColorSettings.active
                }

                QQC2.ToolTip {
                    text: i18n("Input minutes - min. 1, max. 600")
                }
            }

            QQC2.Label {
                id: manualTimingsError
                visible: {
                    var day = 86400000;
                    var trTime = transTimeField.value * 60 * 1000;
                    var mor = mornBeginManField.getNormedDate();
                    var eve = evenBeginManField.getNormedDate();

                    var diffMorEve = eve > mor ? eve - mor : mor - eve;
                    var diffMin = Math.min(diffMorEve, day - diffMorEve);

                    return diffMin <= trTime && kcm.nightColorSettings.active
                        && kcm.nightColorSettings.mode === NightColorMode.Timings;
                }
                font.italic: true
                text: i18n("Error: Transition time overlaps.")
            }
        }

        // Show location chooser in manual location mode
        LocationsFixedView {
            visible: kcm.nightColorSettings.mode === NightColorMode.Location && kcm.nightColorSettings.active
            Layout.alignment: Qt.AlignHCenter
            enabled: kcm.nightColorSettings.active
        }

        Item {
            visible: kcm.nightColorSettings.active
                && kcm.nightColorSettings.mode === NightColorMode.Automatic
                && (!locator || !root.doneLocating)
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
