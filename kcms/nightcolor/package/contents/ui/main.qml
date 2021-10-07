/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.12
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.5 as Kirigami
import org.kde.kcm 1.5 as KCM

import org.kde.colorcorrect 0.1 as CC

import org.kde.private.kcms.nightcolor 1.0

KCM.SimpleKCM {
    id: root
    property int error: cA.error
    property bool defaultRequested: false
    property var locator
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
        root.locator = Qt.createQmlObject('import org.kde.colorcorrect 0.1 as CC; CC.Geolocator {}', root, "geoLocatorObj");
    }

    function endLocator() {
        root.locator.destroy();
    }

    Component.onCompleted: {
        if (kcm.nightColorSettings.mode == CC.CompositorAdaptor.ModeAutomatic && kcm.nightColorSettings.active) {
            startLocator();
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

    ColumnLayout {
        spacing: 0

        QQC2.Label {
            Layout.topMargin: Kirigami.Units.largeSpacing * 2
            Layout.bottomMargin: Kirigami.Units.largeSpacing * 4
            Layout.leftMargin: Kirigami.Units.smallSpacing
            Layout.rightMargin: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: Math.round(root.width * 0.5)

            text: i18n("Night Color makes the colors on the screen warmer to reduce eye strain at the time of your choosing.")
            wrapMode: Text.WordWrap
        }

        Kirigami.FormLayout {
            id: parentLayout

            QQC2.CheckBox {
                id: activator
                text: i18n("Activate Night Color")
                checked: kcm.nightColorSettings.active
                onCheckedChanged: kcm.nightColorSettings.active = checked

                KCM.SettingStateBinding {
                    configObject: kcm.nightColorSettings
                    settingName: "Active"
                    extraEnabledConditions: true//cA.nightColorAvailable
                }
                KCM.SettingHighlighter {
                    highlight: true
                }
            }

            Item {
                Kirigami.FormData.isSection: true
            }

            GridLayout {
                Kirigami.FormData.label: i18n("Night Color Temperature:")
                Kirigami.FormData.buddyFor: tempSlider
                enabled: kcm.nightColorSettings.active

                columns: 4

                QQC2.Slider {
                    id: tempSlider
                    // Match combobox width
                    Layout.minimumWidth: modeSwitcher.width
                    from: 1000 // TODO get min/max fron kcfg
                    to: 6500
                    stepSize: 100

                    value: kcm.nightColorSettings.nightTemperature

                    onMoved: kcm.nightColorSettings.nightTemperature = value

                    Layout.columnSpan: 3

                    KCM.SettingStateBinding {
                        configObject: kcm.nightColorSettings
                        settingName: "NightTemperature"
                        extraEnabledConditions: kcm.nightColorSettings.active
                    }
                }
                QQC2.Label {
                        text: i18nc("Color temperature in Kelvin", "%1K", tempSlider.value)
                }
                //row 2
                QQC2.Label {
                    text: i18nc("Night colour red-ish", "Warm")
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: i18nc("Night colour blue-ish", "Cool")
                }
                Item {}
            }

            Item {
                Kirigami.FormData.isSection: true
            }

            QQC2.ComboBox {
                id: modeSwitcher
                // Work around https://bugs.kde.org/show_bug.cgi?id=403153
                Layout.minimumWidth: Kirigami.Units.gridUnit * 17
                Kirigami.FormData.label: i18n("Activation time:")
                enabled: activator.checked
                model: [
                    i18n("Sunset to sunrise at current location"),
                    i18n("Sunset to sunrise at manual location"),
                    i18n("Custom time"),
                    i18n("Always on")
                ]
                currentIndex: kcm.nightColorSettings.mode
                onCurrentIndexChanged: {
		    kcm.nightColorSettings.mode = currentIndex;
		    if (currentIndex == CC.CompositorAdaptor.ModeAutomatic) {
                        startLocator();
                    } else {
                        endLocator();
                    }
		}

                KCM.SettingStateBinding {
                    configObject: kcm.nightColorSettings
                    settingName: "Mode"
                    extraEnabledConditions: kcm.nightColorSettings.active
                }
            }

            // Inform about geolocation access in auto mode
            QQC2.Label {
                visible: modeSwitcher.currentIndex === CC.CompositorAdaptor.ModeAutomatic
                enabled: activator.checked
                wrapMode: Text.Wrap
                Layout.maximumWidth: modeSwitcher.width
                text: i18n("The device's location will be periodically updated using GPS (if available), or by sending network information to <a href=\"https://location.services.mozilla.com\">Mozilla Location Service</a>.")
                onLinkActivated: { Qt.openUrlExternally("https://location.services.mozilla.com"); }
                font: Kirigami.Theme.smallFont
            }

            // Workaround for Layout.margins not working in Kirigami FormLayout (bug 434625)
            Item { implicitHeight: Kirigami.Units.largeSpacing }

            // Show current location in auto mode
            QQC2.Label {
                visible: kcm.nightColorSettings.mode === NightColorMode.Automatic
                enabled: activator.checked
                wrapMode: Text.Wrap
                text: i18n("Latitude: %1°   Longitude: %2°", Math.round(locator.latitude * 100)/100, Math.round(locator.longitude * 100)/100)
            }

            // Show time entry fields in manual timings mode
            TimeField {
                id: evenBeginManField
                // Match combobox width
                Layout.minimumWidth: modeSwitcher.width
                visible: kcm.nightColorSettings.mode === NightColorMode.Timings
                Kirigami.FormData.label: i18n("Turn on at:")
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
                visible: kcm.nightColorSettings.mode === NightColorMode.Timings
                Kirigami.FormData.label: i18n("Turn off at:")
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
                visible: kcm.nightColorSettings.mode === NightColorMode.Timings
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
                id: manualTimingsError1
                visible: evenBeginManField.getNormedDate() - mornBeginManField.getNormedDate() <= 0
                font.italic: true
                text: i18n("Error: Morning is before evening.")
            }

            QQC2.Label {
                id: manualTimingsError2
                visible: {
                    if (manualTimingsError1.visible) {
                        return false;
                    }
                    var trTime = transTimeField.backend * 60 * 1000;
                    var mor = mornBeginManField.getNormedDate();
                    var eve = evenBeginManField.getNormedDate();

                    return eve - mor <= trTime || eve - mor >= 86400000 - trTime;
                }
                font.italic: true
                text: i18n("Error: Transition time overlaps.")
            }
        }

        // Show location chooser in manual location mode
        LocationsFixedView {
            visible: kcm.nightColorSettings.mode === NightColorMode.Location
            enabled: activator.checked
        }

        // Show start/end times in automatic and manual location modes
        TimingsView {
            id: timings
            visible: kcm.nightColorSettings.mode === NightColorMode.Automatic ||
            kcm.nightColorSettings.mode === NightColorMode.Location
            enabled: kcm.nightColorSettings.active
            latitude: kcm.nightColorSettings.mode === NightColorMode.Automatic ? locator.latitude : kcm.nightColorSettings.latitudeFixed
            longitude: kcm.nightColorSettings.mode === NightColorMode.Automatic ? locator.longitude : kcm.nightColorSettings.longitudeFixed
        }
    }
}
