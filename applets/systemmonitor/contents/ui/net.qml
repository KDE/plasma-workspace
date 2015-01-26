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
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: rootItem

    width: units.gridUnit * 20
    height: units.gridUnit * 10
    Plasmoid.preferredRepresentation: plasmoid.fullRepresentation

    PlasmaCore.DataSource {
        id: smSource

        property var availableNetworks: []
        property var enabledNetworks: []
        //arrays don't signal
        property int enabledNetworksCount: 0

        function updateEnabledNetworks() {
            if (!plasmoid.configuration.interfaces ||
                (plasmoid.configuration.interfaces &&
                 !plasmoid.configuration.interfaces[0])) {
                enabledNetworks = availableNetworks;
            } else {
                for (var i in availableNetworks) {
                    if (plasmoid.configuration.interfaces.indexOf(availableNetworks[i]) != -1) {
                        enabledNetworks.push(availableNetworks[i]);
                    }
                }
            }

            var newSources = [];
            for (var i in enabledNetworks) {
                newSources.push("network/interfaces/" + availableNetworks[i] + "/receiver/data");
                newSources.push("network/interfaces/" + availableNetworks[i] + "/transmitter/data");
            }
            connectedSources = newSources;
            enabledNetworksCount = enabledNetworks.length;
        }

        engine: "systemmonitor"
        interval: 2000
        onSourceAdded: {
            if (source.indexOf("network/interfaces/lo/") !== -1) {
                return;
            }
            var match = source.match(/^network\/interfaces\/(\w+)\/transmitter\/data$/);
            if (match) {
                availableNetworks.push(match[1]);
            }

            updateEnabledNetworks();
        }
        onSourceRemoved: {
            if (source.indexOf("network/interfaces/lo/") !== -1) {
                return;
            }
            var match = source.match(/^network\/interfaces\/(\w+)\/transmitter\/data$/);
            if (match) {
                var idx = availableNetworks.indexOf(match[1]);
                if (idx !== -1) {
                    availableNetworks.splice(idx, 1);
                }
            }

            updateEnabledNetworks();
        }
    }
    Connections {
        target: plasmoid.configuration
        onInterfacesChanged: {
            smSource.updateEnabledNetworks();
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
            model: smSource.enabledNetworksCount

            PlasmaExtras.Plotter {
                id: plotter
                property string ifName: smSource.enabledNetworks[modelData]

                Layout.fillWidth: true
                Layout.fillHeight: true

                dataSets: [
                    PlasmaExtras.PlotData {
                        label: i18n("Download")
                        color: theme.highlightColor
                    },
                    PlasmaExtras.PlotData {
                        label: i18n("Upload")
                        color: cycle(theme.highlightColor, -90)
                    }
                ]

                PlasmaComponents.Label {
                    anchors {
                        left: parent.left
                        top: parent.top
                    }
                    text: plotter.ifName
                }

                PlasmaComponents.Label {
                    id: speedLabel
                    anchors.centerIn: parent
                }

                Connections {
                    target: smSource
                    onNewData: {
                        if (sourceName.indexOf("network/interfaces/" + plotter.ifName) != 0) {
                            return;
                        }
                        var rx = smSource.data["network/interfaces/" + plotter.ifName + "/receiver/data"];
                        var tx = smSource.data["network/interfaces/" + plotter.ifName + "/transmitter/data"];

                        if (rx === undefined || rx.value === undefined ||
                            tx === undefined || tx.value === undefined) {
                            return;
                        }

                        plotter.addSample([rx.value, tx.value]);

                        speedLabel.text = i18n("%1 %2 / %3 %4", rx.value, rx.units,
                                            tx.value, tx.units);
                    }
                }
            }
        }
    }

}
