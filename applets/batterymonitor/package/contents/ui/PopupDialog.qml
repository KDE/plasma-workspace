/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0

PlasmaComponents3.Page {
    id: dialog

    property alias model: batteryList.model
    property bool pluggedIn

    property int remainingTime

    property bool isBrightnessAvailable
    property bool isKeyboardBrightnessAvailable

    signal powermanagementChanged(bool disabled)

    header: PlasmaExtras.PlasmoidHeading {
        PowerManagementItem {
            id: pmSwitch
            width: parent.width
            pluggedIn: dialog.pluggedIn
            onDisabledChanged: powermanagementChanged(disabled)
            KeyNavigation.tab: batteryList
            KeyNavigation.backtab: keyboardBrightnessSlider
        }
    }

    FocusScope {
        anchors.fill: parent
        anchors.topMargin: PlasmaCore.Units.smallSpacing * 2

        focus: true

        Column {
            id: settingsColumn
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - PlasmaCore.Units.smallSpacing * 2
            spacing: Math.round(PlasmaCore.Units.gridUnit / 2)

            BrightnessItem {
                id: brightnessSlider
                width: parent.width

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
                        brightnessSlider.value = batterymonitor.screenBrightness
                    }
                }
            }

            BrightnessItem {
                id: keyboardBrightnessSlider
                width: parent.width

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
                        keyboardBrightnessSlider.value = batterymonitor.keyboardBrightness
                    }
                }
            }
        }

        PlasmaExtras.ScrollArea {
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: settingsColumn.bottom
                topMargin: PlasmaCore.Units.gridUnit
                leftMargin: PlasmaCore.Units.smallSpacing
                bottom: parent.bottom
            }
            width: parent.width - PlasmaCore.Units.smallSpacing * 2

            ListView {
                id: batteryList

                boundsBehavior: Flickable.StopAtBounds
                spacing: Math.round(PlasmaCore.Units.gridUnit / 2)

                KeyNavigation.tab: brightnessSlider
                KeyNavigation.backtab: pmSwitch

                delegate: BatteryItem {
                    width: parent.width
                    battery: model
                }
            }
        }
    }
}
