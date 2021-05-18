/*
*   Copyright 2011 Sebastian Kügler <sebas@kde.org>
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
import org.kde.plasma.workspace.components 2.0 as WorkspaceComponents

MouseArea {
    id: root

    property real itemSize: Math.min(root.height, root.width/view.count)
    readonly property bool isConstrained: plasmoid.formFactor === PlasmaCore.Types.Vertical || plasmoid.formFactor === PlasmaCore.Types.Horizontal
    property real brightnessError: 0


    onClicked: plasmoid.expanded = !plasmoid.expanded
    
    onWheel: {
        var delta = wheel.angleDelta.y || wheel.angleDelta.x

        var maximumBrightness = batterymonitor.maximumScreenBrightness
        // Don't allow the UI to turn off the screen
        // Please see https://git.reviewboard.kde.org/r/122505/ for more information
        var minimumBrightness = (maximumBrightness > 100 ? 1 : 0)
        var stepSize = Math.max(1, maximumBrightness / 20)

        if (Math.abs(delta) < 120) {
            // Touchpad scrolling
            brightnessError += delta * stepSize / 120
            var change = Math.round(brightnessError);
            var newBrightness = batterymonitor.screenBrightness + change
            brightnessError -= change
        } else {
            // Discrete/wheel scrolling
            var newBrightness = Math.round(batterymonitor.screenBrightness/stepSize + delta/120) * stepSize
        }
        batterymonitor.screenBrightness = Math.max(minimumBrightness, Math.min(maximumBrightness, newBrightness));
    }



    //Should we consider turning this into a Flow item?
    Row {
        anchors.centerIn: parent
        Repeater {
            id: view

            property bool hasBattery: batterymonitor.pmSource.data["Battery"]["Has Cumulative"]
            property bool singleBattery: root.isConstrained || !view.hasBattery

            model: singleBattery ? 1 : batterymonitor.batteries

            Item {
                id: batteryContainer

                property bool hasBattery: view.singleBattery ? view.hasBattery : model["Plugged in"]
                property int percent: view.singleBattery ? pmSource.data["Battery"]["Percent"] : model["Percent"]
                property bool pluggedIn: pmSource.data["AC Adapter"] && pmSource.data["AC Adapter"]["Plugged in"] && (view.singleBattery || model["Is Power Supply"])

                height: root.itemSize
                width: root.width/view.count

                property real iconSize: Math.min(width, height)

                WorkspaceComponents.BatteryIcon {
                    id: batteryIcon

                    anchors.centerIn: parent
                    height: root.isConstrained ? batteryContainer.iconSize : batteryContainer.iconSize - batteryLabel.height
                    width: height

                    hasBattery: batteryContainer.hasBattery
                    percent: batteryContainer.percent
                    pluggedIn: batteryContainer.pluggedIn
                }

                BadgeOverlay {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right

                    visible: plasmoid.configuration.showPercentage && batteryContainer.hasBattery

                    text: i18nc("battery percentage below battery icon", "%1%", percent)
                    icon: batteryIcon
                }
            }
        }
    }
}
