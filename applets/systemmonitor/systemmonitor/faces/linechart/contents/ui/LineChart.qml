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

Charts.LineChart {
    id: chart
    
    //property var sensors: root.controller.sensorIds

    readonly property alias sensorsModel: sensorsModel

    Layout.minimumWidth: Kirigami.Units.gridUnit * 8

    direction: Charts.XYChart.ZeroAtEnd

    fillOpacity: root.controller.faceConfiguration.lineChartFillOpacity / 100
    stacked: root.controller.faceConfiguration.lineChartStacked
    smooth: root.controller.faceConfiguration.lineChartSmooth

    //TODO: Have a central heading here too?
    //TODO: Have a plasmoid config value for line thickness?

    xRange {
        from: root.controller.faceConfiguration.rangeFromX
        to: root.controller.faceConfiguration.rangeToX
        automatic: root.controller.faceConfiguration.rangeAutoX
    }
    yRange {
        from: root.controller.faceConfiguration.rangeFromY
        to: root.controller.faceConfiguration.rangeToY
        automatic: root.controller.faceConfiguration.rangeAutoY
    }

    Sensors.SensorDataModel {
        id: sensorsModel
        sensors: root.controller.sensorIds
    }

    Instantiator {
        model: sensorsModel.sensors
        delegate: Charts.ModelHistorySource {
            model: sensorsModel
            column: index
            row: 0
            roleName: "Value"
            maximumHistory: 50
        }
        onObjectAdded: {
            chart.insertValueSource(index, object)
        }
        onObjectRemoved: {
            chart.removeValueSource(object)
        }
    }

    colorSource: root.colorSource
    nameSource: Charts.ModelSource {
        model: sensorsModel
        roleName: "ShortName"
        indexColumns: true
    }
}

