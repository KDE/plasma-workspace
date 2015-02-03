/*
 *   Copyright 2014 Marco Martin <mart@kde.org>
 *
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
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0 as KQuickAddons

Item {
    id: rootItem

    signal sourceAdded(string source)
    property Component delegate

    width: units.gridUnit * 20
    height: units.gridUnit * 10
    Plasmoid.preferredRepresentation: plasmoid.fullRepresentation

    function addSource(source1, friendlyName1, source2, friendlyName2) {
        var found = false;
        for (var i = 0; i < sourcesModel.count; ++i) {
            var obj = sourcesModel.get(i);
            if (obj.source1 == source1 && obj.source2 == source2) {
                found = true;
                break;
            }
        }
        if (found) {
            return;
        }

        smSource.connectSource(source1);
        if (source2) {
            smSource.connectSource(source2);
        }

        sourcesModel.append(
           {"source1": source1,
            "friendlyName1": friendlyName1,
            "source2": source2,
            "friendlyName2": friendlyName2,
            "dataSource": smSource});
    }

    ListModel {
        id: sourcesModel
    }

    Component.onCompleted: {
        for (var i in smSource.sources) {
            smSource.sourceAdded(smSource.sources[i]);
        }
    }

    PlasmaCore.DataSource {
        id: smSource

        engine: "systemmonitor"
        interval: 2000
        onSourceAdded: {
            if (plasmoid.configuration.sources.length > 0 &&
                plasmoid.configuration.sources.indexOf(source) === -1) {
                return;
            }
            rootItem.sourceAdded(source);
        }
        onSourceRemoved: {
            for (var i = sourcesModel.count - 1; i >= 0; --i) {
                var obj = sourcesModel.get(i);
                if (obj.source1 == source || obj.source2 == source) {
                    sourcesModel.remove(i);
                }
            }
            smSource.disconnectSource(source);
        }
    }
    Connections {
        target: plasmoid.configuration
        onSourcesChanged: {
            if (plasmoid.configuration.sources.length == 0) {
                for (var i in smSource.sources) {
                    var source = smSource.sources[i];
                    smSource.sourceAdded(source);
                }

            } else {
                for (var i in plasmoid.configuration.sources) {
                    var source = plasmoid.configuration.sources[i];
                    smSource.sourceAdded(source);
                }

                for (var i = sourcesModel.count - 1; i >= 0; --i) {
                    var obj = sourcesModel.get(i);
                    if (plasmoid.configuration.sources.indexOf(obj.source1) === -1) {
                        smSource.sourceRemoved(obj.source1);
                    }
                }
            }
        }
    }

    function cycle(color, degrees) {
        var min = Math.min(color.r, Math.min(color.g, color.b));
        var max = Math.max(color.r, Math.max(color.g, color.b));
        var c = max - min;
        var h;

        if (c == 0) {
            h = 0
        } else if (max == color.r) {
            h = ((color.g - color.b) / c) % 6;
        } else if (max == color.g) {
            h = ((color.b - color.r) / c) + 2;
        } else if (max == color.b) {
            h = ((color.r - color.g) / c) + 4;
        }
        var hue = (1/6) * h + (degrees/360);
        var saturation = c / (1 - Math.abs(2 * ((max+min)/2) - 1));
        var lightness = (max + min)/2;

        return Qt.hsla(hue, saturation, lightness, 1.0);
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        Repeater {
            model: sourcesModel

            delegate: rootItem.delegate
        }
    }

    Component {
        id: singleValuePlotter
        KQuickAddons.Plotter {
            id: plotter
            property string sensorName: model.friendlyName1

            Layout.fillWidth: true
            Layout.fillHeight: true

            dataSets: [
                KQuickAddons.PlotData {
                    color: theme.highlightColor
                }
            ]

            PlasmaComponents.Label {
                anchors {
                    left: parent.left
                    top: parent.top
                }
                text: plotter.sensorName
            }

            PlasmaComponents.Label {
                id: speedLabel
                anchors.centerIn: parent
            }

            Connections {
                target: smSource
                onNewData: {
                    if (sourceName.indexOf(model.source1) != 0) {
                        return;
                    }

                    var data1 = smSource.data[model.source1];

                    if (data1 === undefined || data1.value === undefined) {
                        return;
                    }

                    plotter.addSample(data1.value);

                    speedLabel.text = i18n("%1 %2", data1.value, data1.units);
                }
            }
        }
    }

    Component {
        id: twoValuesPlotter
        KQuickAddons.Plotter {
            id: plotter
            property string sensorName: model.friendlyName1

            Layout.fillWidth: true
            Layout.fillHeight: true

            dataSets: [
                KQuickAddons.PlotData {
                    color: theme.highlightColor
                },
                KQuickAddons.PlotData {
                    color: cycle(theme.highlightColor, -90)
                }
            ]

            PlasmaComponents.Label {
                anchors {
                    left: parent.left
                    top: parent.top
                }
                text: plotter.sensorName
            }

            PlasmaComponents.Label {
                id: speedLabel
                anchors.centerIn: parent
            }

            Connections {
                target: smSource
                onNewData: {
                    if (sourceName.indexOf(model.source1) != 0 && sourceName.indexOf(model.source2) != 0) {
                        return;
                    }

                    var data1 = smSource.data[model.source2];
                    var data2 = smSource.data[model.source1];

                    if (data1 === undefined || data1.value === undefined ||
                        data2 === undefined || data2.value === undefined) {
                        return;
                    }

                    plotter.addSample([data1.value, data2.value]);

                    speedLabel.text = i18n("%1 %2 / %3 %4", data1.value, data1.units,
                                    data2.value, data2.units);

                }
            }
        }
    }
}
