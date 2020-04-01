 /*
  *   Copyright 2019 Marco Martin <mart@kde.org>
  *   Copyright 2019 David Edmundson <davidedmundson@kde.org>
  *   Copyright 2019 Arjen Hiemstra <ahiemstra@heimr.nl>
  *
  *   This program is free software; you can redistribute it and/or
  *   modify it under the terms of the GNU General Public License as
  *   published by the Free Software Foundation; either version 2 of
  *   the License or (at your option) version 3 or any later version
  *   accepted by the membership of KDE e.V. (or its successor approved
  *   by the membership of KDE e.V.), which shall act as a proxy 
  *   defined in Section 14 of version 3 of the license.
  *
  *   This program is distributed in the hope that it will be useful,
  *   but WITHOUT ANY WARRANTY; without even the implied warranty of
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *   GNU General Public License for more details.
  *
  *   You should have received a copy of the GNU General Public License
  *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */

import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.2

import QtQml.Models 2.12

import org.kde.kirigami 2.2 as Kirigami

import org.kde.ksysguard.sensors 1.0 as Sensors2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.quickcharts 1.0 as Charts

ColumnLayout {
    id: root

    Layout.minimumWidth: Kirigami.Units.gridUnit * 8
    Layout.minimumHeight: Kirigami.Units.gridUnit * 4

    Layout.preferredWidth: Kirigami.Units.gridUnit * 16
    Layout.preferredHeight: Kirigami.Units.gridUnit * 18

    opacity: y + height <= parent.height

    RowLayout {
        Layout.fillHeight: false
        Kirigami.Heading {
            id: heading
            text: plasmoid.configuration.title
            level: 2
            elide: Text.ElideRight
            Layout.fillHeight: true
        }

        Charts.LineChart {
            Layout.fillWidth: true
            Layout.fillHeight: true
            fillOpacity: 0

            xRange { from: 0; to: 50; automatic: false }
            yRange { from: 0; to: 100; automatic: false }

            visible: plasmoid.configuration.totalSensor !== ""

            smooth: true
            colorSource: Charts.SingleValueSource { value: root.Kirigami.Theme.textColor}
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
    }

    RowLayout {
        id: header
        y: -height
        Layout.fillWidth: true
        visible: plasmoid.nativeInterface.faceConfiguration.showTableHeaders
        Repeater {
            model: Sensors2.HeadingHelperModel {
                id: headingHelperModel

                sourceModel: tableView.model
            }
            Kirigami.Heading {
                id: heading
                level: 4
                Layout.fillWidth: true
                //FIXME: why +2 is necessary?
                Layout.preferredWidth: metrics.width+2
                Layout.maximumWidth: x > 0 ? metrics.width+2 : -1
                TextMetrics{
                    id: metrics
                    font: heading.font
                    text: heading.text
                }
                text: model.display
                elide: Text.ElideRight
                horizontalAlignment: index > 0 ? Text.AlignRight : Text.AlignLeft
            }
        }
    }
    TableView {
        id: tableView

        Layout.fillHeight: true
        Layout.fillWidth: true

        model: topModel
        interactive: false
        columnSpacing: Kirigami.Units.smallSpacing
        clip: true

        property int dataColumnWidth: Kirigami.Units.gridUnit * 5

        columnWidthProvider: function (column) {
            return header.children[column].width;
        }

        delegate: Label {
            // Not visible to not change contentHeight
            opacity: y + height <= tableView.height
            text: model.display != undefined ? model.display : ""
            horizontalAlignment: model.alignment == (Text.AlignRight + Text.AlignVCenter) ? Text.AlignRight : Text.AlignLeft
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        //See https://codereview.qt-project.org/c/qt/qtdeclarative/+/262876
        onWidthChanged: Qt.callLater(function() {if (tableView.columns > 0) {
            tableView.forceLayout()
        }});

        PlasmaCore.SortFilterModel {
            id: topModel
            filterKeyColumn: 0 // name
            filterRegExp: ".+"

            sortColumn: plasmoid.nativeInterface.faceConfiguration.sortColumn
            sortRole: "Value"
            sortOrder: plasmoid.nativeInterface.faceConfiguration.sortDescending ? Qt.DescendingOrder : Qt.AscendingOrder

            sourceModel:  Sensors2.SensorDataModel {
                id: dataModel
                sensors: plasmoid.configuration.sensorIds
                onSensorsChanged: {
                    //note: this re sets the models in order to make the table work with any new role
                    tableView.model = null;
                    topModel.sourceModel = null;
                    topModel.sourceModel = dataModel;
                    tableView.model = topModel;
                }
            }
        }
    }
}
