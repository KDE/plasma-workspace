/*
 *   Copyright 2019 Marco Martin <mart@kde.org>
 *   Copyright 2019 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2019 Arjen Hiemstra <ahiemstra@heimr.nl>
 *   Copyright 2019 Kai Uwe Broulik <kde@broulik.de>
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
import org.kde.ksysguard.faces 1.0 as Faces

import org.kde.quickcharts 1.0 as Charts
import org.kde.quickcharts.controls 1.0 as ChartControls


ChartControls.PieChartControl {
    id: chart

    property alias headingSensor: sensor.sensorId
    property alias sensors: sensorsModel.sensors
    property alias sensorsModel: sensorsModel

    Layout.minimumWidth: root.formFactor != Faces.SensorFace.Vertical ? Kirigami.Units.gridUnit * 4 : Kirigami.Units.gridUnit
    Layout.minimumHeight: root.formFactor == Faces.SensorFace.Vertical ? width : Kirigami.Units.gridUnit

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    chart.smoothEnds: root.controller.faceConfiguration.smoothEnds
    chart.fromAngle: root.controller.faceConfiguration.fromAngle
    chart.toAngle: root.controller.faceConfiguration.toAngle

    range {
        from: root.controller.faceConfiguration.rangeFrom
        to: root.controller.faceConfiguration.rangeTo
        automatic: root.controller.faceConfiguration.rangeAuto
    }

    chart.backgroundColor: Qt.rgba(0.0, 0.0, 0.0, 0.2)

    text: sensor.formattedValue
    valueSources: Charts.ModelSource {
        model: Sensors.SensorDataModel {
            id: sensorsModel
            sensors: root.controller.sensorIds
        }
        roleName: "Value"
        indexColumns: true
    }
    chart.nameSource: Charts.ModelSource {
        roleName: "ShortName";
        model: valueSources[0].model;
        indexColumns: true
    }
    chart.colorSource: root.colorSource

    Sensors.Sensor {
        id: sensor
        sensorId: root.controller.totalSensor
    }
}

