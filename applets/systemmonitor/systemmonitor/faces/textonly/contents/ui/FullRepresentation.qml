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

ColumnLayout
{
    id: root

    // Arbitrary minimumWidth to make easier to align plasmoids in a predictable way
    Layout.minimumWidth: Kirigami.Units.gridUnit * 8

    Kirigami.Heading {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        text: plasmoid.configuration.title
        visible: text.length > 0
        level: 2
    }

    Item { Layout.fillWidth: true; Layout.fillHeight: true }

    Repeater {
        model: plasmoid.configuration.sensorIds.concat(plasmoid.configuration.textOnlySensorIds)

        ChartsControls.LegendDelegate {
            readonly property bool isTextOnly: index >= plasmoid.configuration.sensorIds.length

            Layout.fillWidth: true
            Layout.minimumHeight: isTextOnly ? 0 : implicitHeight

            name: sensor.shortName
            value: sensor.formattedValue
            colorVisible: !isTextOnly
            color: !isTextOnly ? globalColorSource.array[index] : "transparent"

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
