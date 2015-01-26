/*
 *  Copyright 2013 Bhushan Shah <bhush94@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Controls 1.1 as Controls
import QtQuick.Layouts 1.1 as Layouts

import org.kde.plasma.core 2.0 as PlasmaCore


Item {
    id: iconsPage
    width: childrenRect.width
    height: childrenRect.height
    implicitWidth: mainColumn.implicitWidth
    implicitHeight: mainColumn.implicitHeight

    property var cfg_interfaces: []

    PlasmaCore.DataSource {
        id: smSource

        property var availableNetworks: []
        //arrays don't signal
        property int availableNetworksCount: 0

        engine: "systemmonitor"

        Component.onCompleted: {
            for (var i in smSource.sources) {
                var source = smSource.sources[i];

                if (source.indexOf("network/interfaces/lo/") !== -1) {
                    continue;
                }
                var match = source.match(/^network\/interfaces\/(\w+)\/transmitter\/data$/);
                if (match) {
                    smSource.availableNetworks.push(match[1]);
                }

                smSource.availableNetworksCount = availableNetworks.length;
            }

            for (var i = 0; i < repeater.count; ++i) {
                if (cfg_interfaces.length == 0 || (cfg_interfaces && !cfg_interfaces[0])) {
                    repeater.itemAt(i).checked = true;
                } else {
                    repeater.itemAt(i).checked = cfg_interfaces.indexOf(mainColumn.children[i].text);
                }
            }
        }
    }

    Layouts.ColumnLayout {
        id: mainColumn

        Repeater {
            id: repeater
            model: smSource.availableNetworksCount
            Controls.CheckBox {
                id: checkBox
                text: smSource.availableNetworks[modelData]
                onCheckedChanged: {
                    if (checked) {
                        if (cfg_interfaces.indexOf(text) == -1) {
                            cfg_interfaces.push(text);
                        }
                    } else {
                        var idx = cfg_interfaces.indexOf(text);
                        if (idx !== -1) {
                            cfg_interfaces.splice(idx, 1);
                        }
                    }
                }
            }
        }
    }
}
