/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2013 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.kquickcontrolsaddons 2.0
import "plasmapackage:/code/logic.js" as Logic

FocusScope {
    id: dialog
    property int implicitHeight: batteryColumn.implicitHeight + settingsColumn.height + separator.height + 10 // 10 = separator margins
    focus: true

    property alias model: batteryList.model
    property bool pluggedIn

    property int remainingTime
    property bool showRemainingTime

    property bool isBrightnessAvailable
    property alias screenBrightness: brightnessSlider.value
    property alias screenBrightnessPercentage: brightnessSlider.percentage

    property bool isKeyboardBrightnessAvailable
    property alias keyboardBrightness: keyboardBrightnessSlider.value
    property alias keyboardBrightnessPercentage: keyboardBrightnessSlider.percentage

    signal brightnessChanged(int screenBrightness)
//    signal keyboardBrightnessChanged(int keyboardBrightness)
    signal powermanagementChanged(bool checked)

    PlasmaCore.FrameSvgItem {
        id: padding
        imagePath: "widgets/viewitem"
        prefix: "hover"
        opacity: 0
    }

    Column {
        id: batteryColumn
        spacing: 4
        width: parent.width
        anchors {
            left: parent.left
            right: parent.right
            top: plasmoid.location == PlasmaCore.Types.BottomEdge ? parent.top : undefined
            bottom: plasmoid.location == PlasmaCore.Types.BottomEdge ? undefined : parent.bottom
            //bottomMargin: parent.height - childrenRect.height
        }

        Repeater {
            id: batteryList

            property int activeIndex

            delegate: BatteryItem { }
            KeyNavigation.tab: brightnessSlider
            KeyNavigation.backtab: pmSwitch

            function updateSelection(old,active) {
                itemAt(old).updateSelection();
                itemAt(active).updateSelection();
            }

            onFocusChanged: {
                var oldIndex = activeIndex;
                activeIndex = 0;
                updateSelection(oldIndex,activeIndex);
            }
            Keys.onDownPressed: {
                var oldIndex = activeIndex;
                activeIndex++;
                if (activeIndex >= model.count) {
                    activeIndex = 0;
                }
                updateSelection(oldIndex,activeIndex);
            }
            Keys.onUpPressed: {
                var oldIndex = activeIndex;
                activeIndex--;
                if (activeIndex < 0) {
                    activeIndex = model.count-1;
                }
                updateSelection(oldIndex,activeIndex);
            }
            Keys.onReturnPressed: itemAt(activeIndex).expanded = !itemAt(activeIndex).expanded
            Keys.onSpacePressed: itemAt(activeIndex).expanded = !itemAt(activeIndex).expanded
            Keys.onLeftPressed: itemAt(activeIndex).expanded = false
            Keys.onRightPressed: itemAt(activeIndex).expanded = true
        }
    }

    Column {
        id: settingsColumn
        spacing: 0
        width: parent.width

        anchors {
            left: parent.left
            right: parent.right
            top: plasmoid.location == PlasmaCore.Types.BottomEdge ? undefined : parent.top
            bottom: plasmoid.location == PlasmaCore.Types.BottomEdge ? parent.bottom : undefined
        }

        BrightnessItem {
            id: brightnessSlider
            icon: "video-display"
            label: i18n("Display Brightness")
            visible: isBrightnessAvailable
            onChanged: brightnessChanged(value)
            KeyNavigation.tab: keyboardBrightnessSlider
            KeyNavigation.backtab: batteryList
            focus: true
        }

        BrightnessItem {
            id: keyboardBrightnessSlider
            icon: "input-keyboard"
            label: i18n("Keyboard Brightness")
            visible: isKeyboardBrightnessAvailable
            onChanged: keyboardBrightnessChanged(value)
            KeyNavigation.tab: pmSwitch
            KeyNavigation.backtab: brightnessSlider
        }

        PowerManagementItem {
            id: pmSwitch
            onEnabledChanged: powermanagementChanged(enabled)
            KeyNavigation.tab: batteryList
            KeyNavigation.backtab: keyboardBrightnessSlider
        }
    }

    Components.Label {
        id: noControlsLabel
        visible: !isKeyboardBrightnessAvailable && !isBrightnessAvailable
        text: i18n("No screen or keyboard brightness controls available")
        wrapMode: Text.Wrap
        anchors {
            top: plasmoid.location == PlasmaCore.Types.BottomEdge ? undefined : settingsColumn.bottom
            bottom: plasmoid.location == PlasmaCore.Types.BottomEdge ? settingsColumn.top : undefined
            left: parent.left
            right: parent.right
            leftMargin: padding.margins.left
            rightMargin: padding.margins.right
            topMargin: 5
            bottomMargin: 5
        }
    }

    PlasmaCore.SvgItem {
        id: separator
        svg: PlasmaCore.Svg {
            id: lineSvg
            imagePath: "widgets/line"
        }
        elementId: "horizontal-line"
        height: lineSvg.elementSize("horizontal-line").height
        width: parent.width
        visible: model.count
        anchors {
            top: plasmoid.location == PlasmaCore.Types.BottomEdge ? undefined : noControlsLabel.bottom
            bottom: plasmoid.location == PlasmaCore.Types.BottomEdge ? noControlsLabel.top : undefined
            leftMargin: padding.margins.left
            rightMargin: padding.margins.right
            topMargin: 5
            bottomMargin: 5
        }
    }
}
