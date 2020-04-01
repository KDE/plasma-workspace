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
    id: root
    
    property var sensors: plasmoid.configuration.sensorIds

    readonly property alias sensorsModel: sensorsModel

    Layout.minimumWidth: Kirigami.Units.gridUnit * 8

    direction: Charts.XYChart.ZeroAtEnd

    fillOpacity: plasmoid.nativeInterface.faceConfiguration.lineChartFillOpacity / 100
    stacked: plasmoid.nativeInterface.faceConfiguration.lineChartStacked
    smooth: plasmoid.nativeInterface.faceConfiguration.lineChartSmooth

    //TODO: Have a central heading here too?
    //TODO: Have a plasmoid config value for line thickness?

    xRange {
        from: plasmoid.nativeInterface.faceConfiguration.rangeFromX
        to: plasmoid.nativeInterface.faceConfiguration.rangeToX
        automatic: plasmoid.nativeInterface.faceConfiguration.rangeAutoX
    }
    yRange {
        from: plasmoid.nativeInterface.faceConfiguration.rangeFromY
        to: plasmoid.nativeInterface.faceConfiguration.rangeToY
        automatic: plasmoid.nativeInterface.faceConfiguration.rangeAutoY
    }

    Sensors.SensorDataModel {
        id: sensorsModel
        sensors: plasmoid.configuration.sensorIds
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
            root.insertValueSource(index, object)
        }
        onObjectRemoved: {
            root.removeValueSource(object)
        }
    }

    colorSource: globalColorSource
    nameSource: Charts.ModelSource {
        model: sensorsModel
        roleName: "ShortName"
        indexColumns: true
    }
}

