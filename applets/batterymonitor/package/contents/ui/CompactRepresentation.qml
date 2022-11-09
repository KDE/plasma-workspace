/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.workspace.components 2.0 as WorkspaceComponents

MouseArea {
    id: root

    property real itemSize: Math.min(root.height, root.width/view.count)
    readonly property bool isConstrained: Plasmoid.formFactor === PlasmaCore.Types.Vertical || Plasmoid.formFactor === PlasmaCore.Types.Horizontal
    property real brightnessError: 0
    property QtObject batteries
    property bool hasBatteries: false
    required property bool isHeldOnPerformanceMode
    required property bool isHeldOnPowerSaveMode
    required property bool isSomehowFullyCharged

    activeFocusOnTab: true
    hoverEnabled: true

    property bool wasExpanded

    Accessible.name: Plasmoid.title
    Accessible.description: `${Plasmoid.toolTipMainText}; ${Plasmoid.toolTipSubText}`
    Accessible.role: Accessible.Button

    onPressed: wasExpanded = Plasmoid.expanded
    onClicked: Plasmoid.expanded = !wasExpanded

    // "No Batteries" case
    PlasmaCore.IconItem {
        anchors.fill: parent
        visible: !root.hasBatteries
        source: Plasmoid.icon
        active: parent.containsMouse
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

                // "Held on a Power Profile mode while plugged in" use case; show the
                // icon of the active mode so the user can notice this at a glance
                PlasmaCore.SvgItem {
                    id: powerProfileModeIcon

                    anchors.centerIn: parent
                    height: batteryContainer.iconSize
                    width: height

                    visible: batteryContainer.pluggedIn && (root.isHeldOnPowerSaveMode || root.isHeldOnPerformanceMode)
                    svg: PlasmaCore.Svg { imagePath: "icons/battery" }
                    elementId: root.isHeldOnPerformanceMode ? "profile-performance" : "profile-powersave"
                }

                // Show normal battery icon
                WorkspaceComponents.BatteryIcon {
                    id: batteryIcon

                    anchors.centerIn: parent
                    height: batteryContainer.iconSize
                    width: height

                    visible: !powerProfileModeIcon.visible
                    hasBattery: root.hasBatteries
                    percent: batteryContainer.percent
                    pluggedIn: batteryContainer.pluggedIn
                }

                WorkspaceComponents.BadgeOverlay {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right

                    visible: Plasmoid.configuration.showPercentage && !root.isSomehowFullyCharged

                    text: i18nc("battery percentage below battery icon", "%1%", percent)
                    icon: batteryIcon
                }
            }
        }
    }
}
