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

import org.kde.ksgrd2 0.1 as KSGRD
import org.kde.quickcharts 1.0 as Charts
import org.kde.quickcharts.controls 1.0 as ChartControls

import org.kde.plasma.core 2.0 as PlasmaCore

ChartControls.PieChartControl {
    id: chart

    property alias headingSensor: sensor.sensorId
    property alias sensors: sensorsModel.sensors
    property alias sensorsModel: sensorsModel

    Layout.minimumWidth: plasmoid.formFactor != PlasmaCore.Types.Vertical ? Kirigami.Units.gridUnit * 4 : Kirigami.Units.gridUnit
    Layout.minimumHeight: plasmoid.formFactor == PlasmaCore.Types.Vertical ? width : Kirigami.Units.gridUnit

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    chart.smoothEnds: plasmoid.nativeInterface.faceConfiguration.smoothEnds
    chart.fromAngle: plasmoid.nativeInterface.faceConfiguration.fromAngle
    chart.toAngle: plasmoid.nativeInterface.faceConfiguration.toAngle

    range {
        from: plasmoid.nativeInterface.faceConfiguration.rangeFrom
        to: plasmoid.nativeInterface.faceConfiguration.rangeTo
        automatic: plasmoid.nativeInterface.faceConfiguration.rangeAuto
    }

    chart.backgroundColor: Qt.rgba(0.0, 0.0, 0.0, 0.2)

    text: sensor.formattedValue
    valueSources: Charts.ModelSource {
        model: KSGRD.SensorDataModel {
            id: sensorsModel
            sensors: plasmoid.configuration.sensorIds
        }
        roleName: "Value"
        indexColumns: true
    }
    chart.nameSource: Charts.ModelSource {
        roleName: "ShortName";
        model: valueSources[0].model;
        indexColumns: true
    }
    chart.colorSource: globalColorSource

    KSGRD.Sensor {
        id: sensor
        sensorId: plasmoid.configuration.totalSensor
    }
}

