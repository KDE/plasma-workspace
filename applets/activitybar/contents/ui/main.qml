/***********************************************************************
 * Copyright 2013 Bhushan Shah <bhush94@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {

    Layout.minimumWidth: tabBar.implicitWidth
    Layout.minimumHeight: tabBar.implicitHeight

    PlasmaCore.DataSource {
        id: activitySource
        engine: "org.kde.activities"
        onSourceAdded: connectSource(source)
    }

    PlasmaComponents.TabBar {
        id: tabBar
        anchors.fill: parent
        rotation: {
            //TODO:  Remove this ugly hack.
            if (plasmoid.formFactor == PlasmaCore.Types.Vertical) {
                return (plasmoid.location == PlasmaCore.Types.LeftEdge) ? 270 : 90;
            } else {
                return 0;
            }
        }

        Repeater {
            model: PlasmaCore.SortFilterModel {
                sourceModel: PlasmaCore.DataModel {
                    dataSource: activitySource
                }
                filterRole: "State"
                filterRegExp: "Running"
            }
            delegate: PlasmaComponents.TabButton {
                id: tab
                text: model["Name"]
                onClicked: {
                    var service = activitySource.serviceForSource(model["DataEngineSource"]);
                    var operation = service.operationDescription("setCurrent");
                    service.startOperationCall(operation);
                }
                Component.onCompleted: {
                    if (model["Current"]) {
                        tabBar.currentTab=tab;
                    }
                }
            }
        }
    }

    function setup() {
        for (var i = 0; i < activitySource.sources.length; i++) {
            if (activitySource.sources[i] != "Status") {
                activitySource.connectSource(activitySource.sources[i]);
            }
        }
    }

    Component.onCompleted: setup()

}
