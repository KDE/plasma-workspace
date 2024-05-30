/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2023-2024 Natalie Clarius <natalie.clarius@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

PlasmaExtras.Representation {
    id: dialog

    KeyNavigation.down: screenBrightnessSlider.Visible ? screenBrightnessSlider : screenBrightnessSlider.KeyNavigation.down

    contentItem: PlasmaComponents3.ScrollView {
        id: scrollView

        focus: false

        function positionViewAtItem(item) {
            if (!PlasmaComponents3.ScrollBar.vertical.visible) {
                return;
            }
            const rect = brightnessList.mapFromItem(item, 0, 0, item.width, item.height);
            if (rect.y < scrollView.contentItem.contentY) {
                scrollView.contentItem.contentY = rect.y;
            } else if (rect.y + rect.height > scrollView.contentItem.contentY + scrollView.height) {
                scrollView.contentItem.contentY = rect.y + rect.height - scrollView.height;
            }
        }

        Column {
            id: brightnessList

            spacing: Kirigami.Units.smallSpacing * 2

            readonly property Item firstHeaderItem: {
                if (screenBrightnessSlider.visible) {
                    return screenBrightnessSlider;
                } else if (keyboardBrightnessSlider.visible) {
                    return keyboardBrightnessSlider;
                }
                return null;
            }
            readonly property Item lastHeaderItem: {
                if (keyboardBrightnessSlider.visible) {
                    return keyboardBrightnessSlider;
                } else if (screenBrightnessSlider.visible) {
                    return screenBrightnessSlider;
                }
                return null;
            }

            BrightnessItem {
                id: screenBrightnessSlider

                width: scrollView.availableWidth

                icon.name: "video-display-brightness"
                text: i18n("Display Brightness")
                type: BrightnessItem.Type.Screen
                visible: screenBrightnessControl.isBrightnessAvailable
                value: screenBrightnessControl.brightness
                maximumValue: screenBrightnessControl.brightnessMax

                KeyNavigation.up: dialog.KeyNavigation.up
                KeyNavigation.down: keyboardBrightnessSlider.visible ? keyboardBrightnessSlider : keyboardBrightnessSlider.KeyNavigation.down
                KeyNavigation.backtab: dialog.KeyNavigation.backtab
                KeyNavigation.tab: KeyNavigation.down

                stepSize: screenBrightnessControl.brightnessMax/100

                onMoved: screenBrightnessControl.brightness = value
                onActiveFocusChanged: if (activeFocus) scrollView.positionViewAtItem(this)

                // Manually dragging the slider around breaks the binding
                Connections {
                    target: screenBrightnessControl
                    function onBrightnessChanged() {
                        screenBrightnessSlider.value = screenBrightnessControl.brightness;
                    }
                }
            }

            BrightnessItem {
                id: keyboardBrightnessSlider

                width: scrollView.availableWidth

                icon.name: "input-keyboard-brightness"
                text: i18n("Keyboard Brightness")
                type: BrightnessItem.Type.Keyboard
                value: keyboardBrightnessControl.brightness
                maximumValue: keyboardBrightnessControl.brightnessMax
                visible: keyboardBrightnessControl.isBrightnessAvailable

                KeyNavigation.up: screenBrightnessSlider.visible ? screenBrightnessSlider : screenBrightnessSlider.KeyNavigation.up
                KeyNavigation.down: keyboardColorItem.visible ? keyboardColorItem : keyboardColorItem.KeyNavigation.down
                KeyNavigation.backtab: KeyNavigation.up
                KeyNavigation.tab: KeyNavigation.down

                onMoved: keyboardBrightnessControl.brightness = value
                onActiveFocusChanged: if (activeFocus) scrollView.positionViewAtItem(this)

                // Manually dragging the slider around breaks the binding
                Connections {
                    target: keyboardBrightnessControl
                    function onBrightnessChanged() {
                        keyboardBrightnessSlider.value = keyboardBrightnessControl.brightness;
                    }
                }
            }

            KeyboardColorItem {
                id: keyboardColorItem

                width: scrollView.availableWidth

                KeyNavigation.up: keyboardBrightnessSlider.visible ? keyboardBrightnessSlider : keyboardBrightnessSlider.KeyNavigation.up
                KeyNavigation.down: nightLightItem
                KeyNavigation.backtab: KeyNavigation.up
                KeyNavigation.tab: KeyNavigation.down

                text: i18n("Keyboard Color")
            }

            NightLightItem {
                id: nightLightItem

                width: scrollView.availableWidth

                KeyNavigation.up: keyboardColorItem.visible ? keyboardColorItem : keyboardColorItem.KeyNavigation.up

                text: i18n("Night Light")
            }

        }
    }
}

