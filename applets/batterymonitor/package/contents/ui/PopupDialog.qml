/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.kquickcontrolsaddons 2.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

PlasmaComponents3.Page {
    id: dialog

    property alias model: batteryList.model
    property bool pluggedIn

    property int remainingTime

    property bool isBrightnessAvailable
    property bool isKeyboardBrightnessAvailable

    property string activeProfile
    property var profiles

    // List of active power management inhibitions (applications that are
    // blocking sleep and screen locking).
    //
    // type: [{
    //  Icon: string,
    //  Name: string,
    //  Reason: string,
    // }]
    property var inhibitions: []

    property string inhibitionReason
    property string degradationReason
    // type: [{ Name: string, Icon: string, Profile: string, Reason: string }]
    required property var profileHolds

    signal powerManagementChanged(bool disabled)
    signal activateProfileRequested(string profile)

    header: PlasmaExtras.PlasmoidHeading {
        PowerManagementItem {
            id: pmSwitch

            anchors {
                left: parent.left
                leftMargin: PlasmaCore.Units.smallSpacing
                right: parent.right
            }

            inhibitions: dialog.inhibitions
            pluggedIn: dialog.pluggedIn
            onDisabledChanged: powerManagementChanged(disabled)

            KeyNavigation.tab: batteryList
            KeyNavigation.backtab: keyboardBrightnessSlider
        }
    }

    FocusScope {
        anchors.fill: parent

        focus: true

        ColumnLayout {
            anchors {
                fill: parent
                topMargin: PlasmaCore.Units.smallSpacing * 2
                leftMargin: PlasmaCore.Units.smallSpacing
                rightMargin: PlasmaCore.Units.smallSpacing
            }
            spacing: PlasmaCore.Units.smallSpacing * 2

            BrightnessItem {
                id: brightnessSlider

                Layout.fillWidth: true

                icon: "video-display-brightness"
                label: i18n("Display Brightness")
                visible: isBrightnessAvailable
                value: batterymonitor.screenBrightness
                maximumValue: batterymonitor.maximumScreenBrightness
                KeyNavigation.tab: keyboardBrightnessSlider
                KeyNavigation.backtab: batteryList
                stepSize: batterymonitor.maximumScreenBrightness/100

                onMoved: batterymonitor.screenBrightness = value

                // Manually dragging the slider around breaks the binding
                Connections {
                    target: batterymonitor
                    function onScreenBrightnessChanged() {
                        brightnessSlider.value = batterymonitor.screenBrightness;
                    }
                }
            }

            BrightnessItem {
                id: keyboardBrightnessSlider

                Layout.fillWidth: true

                icon: "input-keyboard-brightness"
                label: i18n("Keyboard Brightness")
                showPercentage: false
                value: batterymonitor.keyboardBrightness
                maximumValue: batterymonitor.maximumKeyboardBrightness
                visible: isKeyboardBrightnessAvailable
                KeyNavigation.tab: pmSwitch
                KeyNavigation.backtab: brightnessSlider

                onMoved: batterymonitor.keyboardBrightness = value

                // Manually dragging the slider around breaks the binding
                Connections {
                    target: batterymonitor
                    function onKeyboardBrightnessChanged() {
                        keyboardBrightnessSlider.value = batterymonitor.keyboardBrightness;
                    }
                }
            }

            PowerProfileItem {
                Layout.fillWidth: true

                activeProfile: dialog.activeProfile
                inhibitionReason: dialog.inhibitionReason
                visible: dialog.profiles.length > 0
                degradationReason: dialog.degradationReason
                profileHolds: dialog.profileHolds
                onActivateProfileRequested: dialog.activateProfileRequested(profile)
            }

            PlasmaComponents3.ScrollView {
                id: batteryScrollView

                // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
                PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

                // additional margin, because the bottom of PowerProfileItem
                // and the top of BatteryItem are more dense.
                Layout.topMargin: PlasmaCore.Units.smallSpacing * 2
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: batteryList

                    boundsBehavior: Flickable.StopAtBounds
                    spacing: PlasmaCore.Units.smallSpacing * 2

                    KeyNavigation.tab: brightnessSlider
                    KeyNavigation.backtab: pmSwitch

                    delegate: BatteryItem {
                        width: ListView.view.width
                        battery: model
                        remainingTime: dialog.remainingTime
                        matchHeightOfSlider: brightnessSlider.slider
                    }
                }
            }
        }
    }
}
