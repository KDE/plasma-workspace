/*
 *   Copyright 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
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
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.kcoreaddons 1.0 as KCoreAddons
import "plasmapackage:/code/logic.js" as Logic

Item {
    id: batteryItem
    clip: true
    width: batteryColumn.width
    height: expanded ? batteryInfos.height + padding.margins.top + padding.margins.bottom * 2 + actionRow.height
                     : batteryInfos.height + padding.margins.top + padding.margins.bottom

    Behavior on height { PropertyAnimation { duration: units.shortDuration * 2 } }

    property bool expanded

    // NOTE: According to the UPower spec this property is only valid for primary batteries, however
    // UPower seems to set the Present property false when a device is added but not probed yet
    property bool isPresent: model["Plugged in"]

    property int remainingTime

    function updateSelection() {
        var hasFocus = batteryList.activeFocus && batteryList.activeIndex == index;
        var containsMouse = mouseArea.containsMouse;

        if (expanded && (hasFocus || containsMouse)) {
            padding.opacity = 1;
        } else if (expanded) {
            padding.opacity = 0.8;
        } else if (hasFocus || containsMouse) {
            padding.opacity = 0.65;
        } else {
            padding.opacity = 0;
        }
    }

    KCoreAddons.Formats {
        id: formats
    }

    PlasmaCore.FrameSvgItem {
        id: padding
        imagePath: "widgets/viewitem"
        prefix: "hover"
        opacity: 0
        Behavior on opacity { PropertyAnimation {} }
        anchors.fill: parent
    }

    onExpandedChanged: updateSelection()

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onEntered: updateSelection()
        onExited: updateSelection()
        onClicked: {
            var oldIndex = batteryList.activeIndex
            batteryList.forceActiveFocus()
            batteryList.activeIndex = index
            batteryList.updateSelection(oldIndex,index)
            expanded = !expanded
        }
    }

    Item {
        id: batteryInfos
        height: Math.max(batteryIcon.height, batteryNameLabel.height + batteryPercentBar.height)

        anchors {
            top: parent.top
            topMargin: padding.margins.top
            left: parent.left
            leftMargin: padding.margins.left
              right: parent.right
            rightMargin: padding.margins.right
        }

        KQuickControlsAddons.QIconItem {
            id: batteryIcon
            width: units.iconSizes.medium
            height: width
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
            }
            icon: Logic.iconForBattery(model,pluggedIn)
        }

        SequentialAnimation {
          id: chargeAnimation
          running: units.longDuration > 0 && model["State"] == "Charging" && model["Is Power Supply"]
          alwaysRunToEnd: true
          loops: Animation.Infinite

          NumberAnimation {
              target: batteryIcon
              properties: "opacity"
              from: 1.0
              to: 0.5
              duration: 750
              easing.type: Easing.InCubic
          }
          NumberAnimation {
              target: batteryIcon
              properties: "opacity"
              from: 0.5
              to: 1.0
              duration: 750
              easing.type: Easing.OutCubic
          }
        }

        Components.Label {
            id: batteryNameLabel
            anchors {
                verticalCenter: isPresent ? undefined : batteryIcon.verticalCenter
                top: isPresent ? parent.top : undefined
                left: batteryIcon.right
                leftMargin: 6
            }
            height: implicitHeight
            elide: Text.ElideRight
            text: model["Pretty Name"]
        }

        Components.Label {
            id: batteryStatusLabel
            anchors {
                top: batteryNameLabel.top
                left: batteryNameLabel.right
                leftMargin: 3
            }
            text: Logic.stringForBatteryState(model)
            height: implicitHeight
            visible: model["Is Power Supply"]
            color: "#77"+(theme.textColor.toString().substr(1))
        }

        Components.ProgressBar {
            id: batteryPercentBar
            anchors {
                bottom: parent.bottom
                left: batteryIcon.right
                leftMargin: 6
                right: batteryPercent.left
                rightMargin: 6
            }
            minimumValue: 0
            maximumValue: 100
            visible: isPresent
            value: parseInt(model["Percent"])
        }

        Components.Label {
            id: batteryPercent
            anchors {
                verticalCenter: batteryPercentBar.verticalCenter
                right: parent.right
            }
            visible: isPresent
            text: i18nc("Placeholder is battery percentage", "%1%", model["Percent"])
        }
    }

    Column {
        id: actionRow
        opacity: expanded ? 1 : 0
        width: parent.width
        anchors {
          top: batteryInfos.bottom
          topMargin: padding.margins.bottom
          left: parent.left
          leftMargin: padding.margins.left
          right: parent.right
          rightMargin: padding.margins.right
          bottomMargin: padding.margins.bottom
        }
        spacing: 4
        Behavior on opacity { PropertyAnimation {} }

        PlasmaCore.SvgItem {
            svg: PlasmaCore.Svg {
                id: lineSvg
                imagePath: "widgets/line"
            }
            elementId: "horizontal-line"
            height: lineSvg.elementSize("horizontal-line").height
            width: parent.width
        }

        Row {
            id: detailsRow
            width: parent.width
            spacing: 4

            Column {
                id: labelsColumn

                DetailsLabel {
                    // FIXME Bound to AC adapter plugged in, not battery charging, see below
                    text: pluggedIn ? i18n("Time To Full:") : i18n("Time To Empty:")
                    horizontalAlignment: Text.AlignRight
                    visible: remainingTimeLabel.visible
                }
                DetailsLabel {
                    text: i18n("Capacity:")
                    horizontalAlignment: Text.AlignRight
                    visible: capacityLabel.visible
                }
                DetailsLabel {
                    text: i18n("Vendor:")
                    horizontalAlignment: Text.AlignRight
                    visible: vendorLabel.visible
                }
                DetailsLabel {
                    text: i18n("Model:")
                    horizontalAlignment: Text.AlignRight
                    visible: modelLabel.visible
                }
            }

            Column {
                width: parent.width - labelsColumn.width - parent.spacing * 2
                DetailsLabel {
                    id: remainingTimeLabel
                    // FIXME Uses overall remaining time, not bound to individual battery
                    text: formats.formatSpelloutDuration (dialogItem.remainingTime)
                    visible: model["Is Power Supply"] && model["State"] != "NoCharge" && text != ""
                    //visible: showRemainingTime && model["Is Power Supply"] && model["State"] != "NoCharge" && text != "" && dialogItem.remainingTime > 0
                }
                DetailsLabel {
                    id: capacityLabel
                    text: i18nc("Placeholder is battery capacity", "%1%", model["Capacity"])
                    visible: model["Is Power Supply"] &&  model["Capacity"] != "" && typeof model["Capacity"] == "number"
                }
                DetailsLabel {
                    id: vendorLabel
                    text: model["Vendor"]
                    visible: model["Vendor"] != "" && typeof model["Vendor"] == "string"
                }
                DetailsLabel {
                    id: modelLabel
                    text: model["Product"]
                    visible: model["Product"] != "" && typeof model["Product"] == "string"
                }
            }
        }
    }
}

