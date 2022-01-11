/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.workspace.components 2.0 as WorkspaceComponents

MouseArea {
    id: root

    property real itemSize: Math.min(root.height, root.width/view.count)
    readonly property bool isConstrained: plasmoid.formFactor === PlasmaCore.Types.Vertical || plasmoid.formFactor === PlasmaCore.Types.Horizontal
    property real brightnessError: 0
    property bool hasBatteries: true

    hoverEnabled: true

    onClicked: plasmoid.expanded = !plasmoid.expanded

    onWheel: {
        const delta = wheel.angleDelta.y || wheel.angleDelta.x

        const maximumBrightness = batterymonitor.maximumScreenBrightness
        // Don't allow the UI to turn off the screen
        // Please see https://git.reviewboard.kde.org/r/122505/ for more information
        const minimumBrightness = (maximumBrightness > 100 ? 1 : 0)
        const stepSize = Math.max(1, maximumBrightness / 20)

        let newBrightness;
        if (Math.abs(delta) < 120) {
            // Touchpad scrolling
            brightnessError += delta * stepSize / 120;
            const change = Math.round(brightnessError);
            brightnessError -= change;
            newBrightness = batterymonitor.screenBrightness + change;
        } else {
            // Discrete/wheel scrolling
            newBrightness = Math.round(batterymonitor.screenBrightness/stepSize + delta/120) * stepSize;
        }
        batterymonitor.screenBrightness = Math.max(minimumBrightness, Math.min(maximumBrightness, newBrightness));
    }

    // "No Batteries" case
    PlasmaCore.IconItem {
        anchors.fill: parent
        visible: !root.hasBatteries
        source: plasmoid.icon
        active: parent.containsMouse
    }

    // We have any batteries; show their status
    //Should we consider turning this into a Flow item?
    Row {
        visible: root.hasBatteries
        anchors.centerIn: parent
        Repeater {
            id: view

            model: root.isConstrained ? 1 : batterymonitor.batteries

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

                    hasBattery: root.hasBatteries
                    percent: batteryContainer.percent
                    pluggedIn: batteryContainer.pluggedIn
                }

                BadgeOverlay {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right

                    visible: plasmoid.configuration.showPercentage

                    text: i18nc("battery percentage below battery icon", "%1%", percent)
                    icon: batteryIcon
                }
            }
        }
    }
}
