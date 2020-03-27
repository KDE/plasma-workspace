/*
 *   Copyright 2019 Marco Martin <mart@kde.org>
 *   Copyright 2019 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2019 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
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

import QtQuick 2.9
import QtQuick.Layouts 1.1

import org.kde.kirigami 2.8 as Kirigami

import org.kde.ksysguard.sensors 1.0 as Sensors
import org.kde.quickcharts 1.0 as Charts
import org.kde.quickcharts.controls 1.0 as ChartsControls
import org.kde.plasma.core 2.0 as PlasmaCore

ColumnLayout
{
    id: root

    Layout.minimumWidth: plasmoid.formFactor == PlasmaCore.Types.Vertical ? Kirigami.Units.gridUnit : Kirigami.Units.gridUnit * 2

    Repeater {
        model: plasmoid.configuration.sensorIds

        ChartsControls.LegendDelegate {
            Layout.fillWidth: true

            name: sensor.shortName
            value: sensor.formattedValue
            colorVisible: false
            colorWidth: 0

            layoutWidth: root.width
            valueWidth: Kirigami.Units.gridUnit * 2

            Sensors.Sensor {
                id: sensor
                sensorId: modelData
            }
        }
    }

    Item { Layout.fillWidth: true; Layout.fillHeight: true }
}
