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

PlasmaExtras.Representation {
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
    property bool inhibitsLidAction

    property string inhibitionReason
    property string degradationReason
    // type: [{ Name: string, Icon: string, Profile: string, Reason: string }]
    required property var profileHolds

    signal powerManagementChanged(bool disabled)
    signal activateProfileRequested(string profile)

    collapseMarginsHint: true

    header: PlasmaExtras.PlasmoidHeading {
        leftPadding: PlasmaCore.Units.smallSpacing
        contentItem: PowerManagementItem {
            id: pmSwitch

            inhibitions: dialog.inhibitions
            inhibitsLidAction: dialog.inhibitsLidAction
            pluggedIn: dialog.pluggedIn
            onDisabledChanged: powerManagementChanged(disabled)

            KeyNavigation.tab: if (batteryList.headerItem) {
                if (isBrightnessAvailable) {
                    return batteryList.headerItem.children[1];
                } else if (isKeyboardBrightnessAvailable) {
                    return batteryList.headerItem.children[2];
                } else if (dialog.profiles.length > 0) {
                    return batteryList.headerItem.children[3];
                } else {
                    return batteryList;
                }
            }
        }
    }

    PlasmaComponents3.ScrollView {
        focus: true
        anchors.fill: parent

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        contentItem: ListView {
            id: batteryList
            keyNavigationEnabled: true

            leftMargin: PlasmaCore.Units.smallSpacing * 2
            rightMargin: PlasmaCore.Units.smallSpacing * 2
            topMargin: PlasmaCore.Units.smallSpacing * 2
            bottomMargin: PlasmaCore.Units.smallSpacing * 2
            spacing: PlasmaCore.Units.smallSpacing

            // header so that it scroll with the content of the ListView
            header: ColumnLayout {
                spacing: PlasmaCore.Units.smallSpacing * 2
                width: parent.width

                BrightnessItem {
                    id: brightnessSlider

                    Layout.fillWidth: true

                    icon: "video-display-brightness"
                    label: i18n("Display Brightness")
                    visible: isBrightnessAvailable
                    value: batterymonitor.screenBrightness
                    maximumValue: batterymonitor.maximumScreenBrightness
                    KeyNavigation.tab: if (isKeyboardBrightnessAvailable) {
                        return keyboardBrightnessSlider;
                    } else if (dialog.profiles.length > 0) {
                        return powerProfileItem
                    } else {
                        return batteryList
                    }
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
                    KeyNavigation.tab: if (dialog.profiles.length > 0) {
                        return powerProfileItem
                    } else {
                        return batteryList
                    }

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
                    id: powerProfileItem
                    Layout.fillWidth: true

                    KeyNavigation.tab: batteryList
                    activeProfile: dialog.activeProfile
                    inhibitionReason: dialog.inhibitionReason
                    visible: dialog.profiles.length > 0
                    degradationReason: dialog.degradationReason
                    profileHolds: dialog.profileHolds
                    onActivateProfileRequested: dialog.activateProfileRequested(profile)
                }
                Item {
                    Layout.fillWidth: true
                    // additional margin, because the bottom of PowerProfileItem
                    // and the top of BatteryItem are more dense.
                }
            }

            delegate: BatteryItem {
                width: ListView.view.width - PlasmaCore.Units.smallSpacing * 4

                battery: model
                remainingTime: dialog.remainingTime
                matchHeightOfSlider: ListView.view.headerItem ? ListView.view.headerItem.children[1].slider : null
            }
        }
    }
}
