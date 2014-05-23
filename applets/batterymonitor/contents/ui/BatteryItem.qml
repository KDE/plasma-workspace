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
    height: expanded ? batteryInfos.height + units.gridUnit + actionRow.height : batteryInfos.height

    Behavior on height { PropertyAnimation { duration: units.shortDuration * 2 } }

    property bool expanded

    // NOTE: According to the UPower spec this property is only valid for primary batteries, however
    // UPower seems to set the Present property false when a device is added but not probed yet
    property bool isPresent: model["Plugged in"]
    property bool charging: model["State"] == "Charging" && model["Is Power Supply"]
    property int remainingTime

    function updateSelection(oldIndex, newIndex) {
        // no-op, since batteryItem doesn't use the focus
        return;
    }

    KCoreAddons.Formats {
        id: formats
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            var oldIndex = batteryList.activeIndex
            batteryList.forceActiveFocus()
            batteryList.activeIndex = index
            expanded = !expanded
        }
    }

    Item {
        id: batteryInfos
        height: Math.max(batteryIcon.height, batteryNameLabel.height + batteryPercentBar.height)

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        KQuickControlsAddons.QIconItem {
            id: batteryIcon
            width: units.iconSizes.medium
            height: width
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
            }
            icon: Logic.iconForBattery(model, charging)
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
                leftMargin: units.gridUnit
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
                leftMargin: units.gridUnit / 3
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
                left: batteryNameLabel.left
                right: parent.right
                rightMargin: units.gridUnit * 2.5
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
                rightMargin: units.gridUnit / 2
            }
            visible: isPresent
            text: i18nc("Placeholder is battery percentage", "%1%", model["Percent"])
        }
    }

    Column {
        id: actionRow
        opacity: expanded ? 1 : 0
        anchors {
          top: batteryInfos.bottom
          left: parent.left
          leftMargin: batteryNameLabel.x
          right: parent.right
          margins: units.gridUnit / 2
        }
        spacing: units.gridUnit / 2
        Behavior on opacity { PropertyAnimation {} }

        Row {
            id: detailsRow
            width: parent.width
            spacing: units.gridUnit / 4

            Column {
                id: labelsColumn

                DetailsLabel {
                    text: charging ? i18n("Time To Full:") : i18n("Time To Empty:")
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

