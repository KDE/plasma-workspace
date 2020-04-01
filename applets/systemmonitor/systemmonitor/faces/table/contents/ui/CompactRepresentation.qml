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

import org.kde.ksysguard.sensors 1.0 as Sensors2
import org.kde.quickcharts 1.0 as Charts
import org.kde.plasma.core 2.0 as PlasmaCore

Charts.LineChart {

    Layout.minimumWidth: Kirigami.Units.gridUnit * 8
    fillOpacity: 0

    xRange { from: 0; to: 50; automatic: false }
    yRange { from: 0; to: 100; automatic: false }

    visible: plasmoid.configuration.totalSensor !== ""

    colorSource: Charts.SingleValueSource { value: Kirigami.Theme.textColor}
    lineWidth: 1
    direction: Charts.XYChart.ZeroAtEnd

    valueSources: [
        Charts.ModelHistorySource {
            model: Sensors2.SensorDataModel { sensors: [ plasmoid.configuration.totalSensor ] }
            column: 0;
            row: 0
            roleName: "Value";
            maximumHistory: 50
        }
    ]
}
