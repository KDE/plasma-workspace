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
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.workspace.components 2.0

MouseArea {
    id: root
    Layout.minimumWidth: units.iconSizes.small * view.count
    Layout.minimumHeight: units.iconSizes.small
    property real itemSize: Math.min(root.height, root.width/view.count)

    onClicked: plasmoid.expanded = !plasmoid.expanded

    readonly property bool isConstrained: plasmoid.formFactor === PlasmaCore.Types.Vertical || plasmoid.formFactor === PlasmaCore.Types.Horizontal

    //Should we consider turning this into a Flow item?
    Row {
        anchors.centerIn: parent
        Repeater {
            id: view

            property bool hasBattery: batterymonitor.pmSource.data["Battery"]["Has Cumulative"]

            property bool singleBattery: isConstrained || !hasBattery

            model: singleBattery ? 1 : batterymonitor.batteries

            Item {
                id: batteryContainer

                property bool hasBattery: view.singleBattery ? view.hasBattery : model["Plugged in"]
                property int percent: view.singleBattery ? pmSource.data["Battery"]["Percent"] : model["Percent"]
                property bool pluggedIn: pmSource.data["AC Adapter"] && pmSource.data["AC Adapter"]["Plugged in"] && (view.singleBattery || model["Is Power Supply"])

                height: root.itemSize
                width: root.width/view.count

                property real iconSize: Math.min(width, height)

                BatteryIcon {
                    id: batteryIcon
                    anchors.centerIn: parent
                    hasBattery: batteryContainer.hasBattery
                    percent: batteryContainer.percent
                    pluggedIn: batteryContainer.pluggedIn
                    height: isConstrained ? batteryContainer.iconSize : batteryContainer.iconSize - batteryLabel.height
                    width: height
                }

                BadgeOverlay {
                    anchors.fill: batteryIcon
                    text: batteryContainer.hasBattery ? i18nc("battery percentage below battery icon", "%1%", percent) : i18nc("short symbol to signal there is no battery currently available", "-")
                    icon: batteryIcon
                    visible: plasmoid.configuration.showPercentage
                }
            }
        }
    }
}
