/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2023 Natalie Clarius <natalie.clarius@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.workspace.components as WorkspaceComponents
import org.kde.kirigami as Kirigami

MouseArea {
    id: root

    property real itemSize: Math.min(root.height, root.width/view.count)
    readonly property bool isConstrained: Plasmoid.formFactor === PlasmaCore.Types.Vertical || Plasmoid.formFactor === PlasmaCore.Types.Horizontal
    property real brightnessError: 0
    property QtObject batteries
    property bool hasBatteries: false
    required property bool isSomehowInPerformanceMode
    required property bool isSomehowInPowerSaveMode
    required property bool isSomehowInhibited
    required property bool isSomehowFullyCharged
    required property bool isPluggedIn
    required property bool isDischarging

    activeFocusOnTab: true
    hoverEnabled: true

    property bool wasExpanded

    Accessible.name: Plasmoid.title
    Accessible.description: `${toolTipMainText}; ${toolTipSubText}`
    Accessible.role: Accessible.Button

    onPressed: wasExpanded = batterymonitor.expanded
    onClicked: batterymonitor.expanded = !wasExpanded

    // "No Batteries" case
    Kirigami.Icon {
        anchors.fill: parent
        visible: !root.hasBatteries
        source: root.isSomehowInhibited ? "speedometer-symbolic" : root.isSomehowInPerformanceMode ? "battery-profile-performance-symbolic" : root.isSomehowInPowerSaveMode ? "battery-profile-powersave-symbolic" : "speedometer-symbolic"
        active: root.containsMouse
    }

    // We have any batteries; show their status
    //Should we consider turning this into a Flow item?
    Row {
        visible: root.hasBatteries
        anchors.centerIn: parent
        Repeater {
            id: view

            model: root.isConstrained ? 1 : root.batteries

            Item {
                id: batteryContainer

                property int percent: root.isConstrained ? pmSource.data["Battery"]["Percent"] : model["Percent"]
                property bool pluggedIn: pmSource.data["AC Adapter"] && pmSource.data["AC Adapter"]["Plugged in"] && (root.isConstrained || model["Is Power Supply"])

                height: root.itemSize
                width: root.width/view.count

                property real iconSize: Math.min(width, height)

                WorkspaceComponents.BatteryIcon {
                    id: batteryIcon

                    anchors.centerIn: parent
                    height: batteryContainer.iconSize
                    width: height

                    active: root.containsMouse
                    hasBattery: root.hasBatteries
                    percent: batteryContainer.percent
                    pluggedIn: batteryContainer.pluggedIn
                }

                WorkspaceComponents.BadgeOverlay {
                    id: percentageOverlay

                    anchors.bottom: parent.bottom
                    anchors.right: parent.right

                    visible: Plasmoid.configuration.showPercentage && !root.isSomehowFullyCharged

                    text: i18nc("battery percentage below battery icon", "%1%", percent)
                    icon: batteryIcon
                }

                Rectangle {
                    id: statusOverlay

                    visible: root.isDischarging && !percentageOverlay.visible && (root.isSomehowInPerformanceMode || root.isSomehowInPowerSaveMode || root.isSomehowInhibited)

                    color: Kirigami.Theme.backgroundColor
                    width: batteryContainer.iconSize * 0.6
                    height: overlayIcon.height
                    radius: 3
                    opacity: 0.9
                        
                    anchors.top: parent.top
                    anchors.right: parent.right

                    Kirigami.Icon {
                        id: overlayIcon

                        height: parent.width
                        width: height
                        anchors.centerIn: parent

                        source: root.isSomehowInhibited ? "speedometer-symbolic" : root.isSomehowInPerformanceMode ? "battery-profile-performance-symbolic" : root.isSomehowInPowerSaveMode ? "battery-profile-powersave-symbolic" : "speedometer-symbolic"
                    }

                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 0
                        radius: 2
                        samples: radius * 2
                        color: Qt.rgba(0, 0, 0, 0.5)
                    }
                }


            }
        }
    }
}
