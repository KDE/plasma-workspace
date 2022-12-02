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

    property alias model: batteryRepeater.model
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

    KeyNavigation.down: pmSwitch.pmCheckBox

    header: PlasmaExtras.PlasmoidHeading {
        leftPadding: PlasmaCore.Units.smallSpacing
        contentItem: PowerManagementItem {
            id: pmSwitch

            inhibitions: dialog.inhibitions
            inhibitsLidAction: dialog.inhibitsLidAction
            pluggedIn: dialog.pluggedIn
            onDisabledChanged: dialog.powerManagementChanged(disabled)
        }
    }

    contentItem: PlasmaComponents3.ScrollView {
        id: scrollView

        focus: false

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        function positionViewAtItem(item) {
            if (!PlasmaComponents3.ScrollBar.vertical.visible) {
                return;
            }
            const rect = batteryList.mapFromItem(item, 0, 0, item.width, item.height);
            if (rect.y < scrollView.contentItem.contentY) {
                scrollView.contentItem.contentY = rect.y;
            } else if (rect.y + rect.height > scrollView.contentItem.contentY + scrollView.height) {
                scrollView.contentItem.contentY = rect.y + rect.height - scrollView.height;
            }
        }

        Column {
            id: batteryList

            spacing: PlasmaCore.Units.smallSpacing * 2

            readonly property Item firstHeaderItem: {
                if (brightnessSlider.visible) {
                    return brightnessSlider;
                } else if (keyboardBrightnessSlider.visible) {
                    return keyboardBrightnessSlider;
                } else if (powerProfileItem.visible) {
                    return powerProfileItem;
                }
                return null;
            }
            readonly property Item lastHeaderItem: {
                if (powerProfileItem.visible) {
                    return powerProfileItem;
                } else if (keyboardBrightnessSlider.visible) {
                    return keyboardBrightnessSlider;
                } else if (brightnessSlider.visible) {
                    return brightnessSlider;
                }
                return null;
            }

            BrightnessItem {
                id: brightnessSlider

                width: scrollView.availableWidth

                icon.name: "video-display-brightness"
                text: i18n("Display Brightness")
                visible: dialog.isBrightnessAvailable
                value: batterymonitor.screenBrightness
                maximumValue: batterymonitor.maximumScreenBrightness

                KeyNavigation.up: pmSwitch.pmCheckBox
                KeyNavigation.down: keyboardBrightnessSlider.visible ? keyboardBrightnessSlider : keyboardBrightnessSlider.KeyNavigation.down
                KeyNavigation.backtab: KeyNavigation.up
                KeyNavigation.tab: KeyNavigation.down
                stepSize: batterymonitor.maximumScreenBrightness/100

                onMoved: batterymonitor.screenBrightness = value
                onActiveFocusChanged: if (activeFocus) scrollView.positionViewAtItem(this)

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

                width: scrollView.availableWidth

                icon.name: "input-keyboard-brightness"
                text: i18n("Keyboard Brightness")
                showPercentage: false
                value: batterymonitor.keyboardBrightness
                maximumValue: batterymonitor.maximumKeyboardBrightness
                visible: dialog.isKeyboardBrightnessAvailable

                KeyNavigation.up: brightnessSlider.visible ? brightnessSlider : brightnessSlider.KeyNavigation.up
                KeyNavigation.down: powerProfileItem.visible ? powerProfileItem : powerProfileItem.KeyNavigation.down
                KeyNavigation.backtab: KeyNavigation.up
                KeyNavigation.tab: KeyNavigation.down

                onMoved: batterymonitor.keyboardBrightness = value
                onActiveFocusChanged: if (activeFocus) scrollView.positionViewAtItem(this)

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

                width: scrollView.availableWidth

                KeyNavigation.up: keyboardBrightnessSlider.visible ? keyboardBrightnessSlider : keyboardBrightnessSlider.KeyNavigation.up
                KeyNavigation.down: batteryRepeater.count > 0 ? batteryRepeater.itemAt(0) : null
                KeyNavigation.backtab: KeyNavigation.up
                KeyNavigation.tab: KeyNavigation.down

                activeProfile: dialog.activeProfile
                inhibitionReason: dialog.inhibitionReason
                visible: dialog.profiles.length > 0
                degradationReason: dialog.degradationReason
                profileHolds: dialog.profileHolds
                onActivateProfileRequested: dialog.activateProfileRequested(profile)

                onActiveFocusChanged: if (activeFocus) scrollView.positionViewAtItem(this)
            }

            Repeater {
                id: batteryRepeater

                delegate: BatteryItem {
                    width: scrollView.availableWidth

                    battery: model
                    remainingTime: dialog.remainingTime
                    matchHeightOfSlider: brightnessSlider.slider

                    KeyNavigation.up: index === 0 ? batteryList.lastHeaderItem : batteryRepeater.itemAt(index - 1)
                    KeyNavigation.down: index + 1 < batteryRepeater.count ? batteryRepeater.itemAt(index + 1) : null
                    KeyNavigation.backtab: KeyNavigation.up
                    KeyNavigation.tab: KeyNavigation.down

                    Keys.onTabPressed: {
                        if (index === batteryRepeater.count - 1) {
                            // Workaround to leave applet's focus on desktop
                            nextItemInFocusChain(false).forceActiveFocus(Qt.TabFocusReason);
                        } else {
                            event.accepted = false;
                        }
                    }

                    onActiveFocusChanged: if (activeFocus) scrollView.positionViewAtItem(this)
                }
            }
        }
    }
}

