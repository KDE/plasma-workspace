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
import org.kde.plasma.components 2.0 as PlasmaComponents // PC3 Tabbar only has top and bottom tab positions, not left and right
import org.kde.activities 0.1 as Activities

Item {

    Layout.minimumWidth: tabBar.implicitWidth
    Layout.minimumHeight: tabBar.implicitHeight
    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation

    PlasmaComponents.TabBar {
        id: tabBar
        anchors.fill: parent
        tabPosition: {
            switch (plasmoid.location) {
            case PlasmaCore.Types.LeftEdge:
                return Qt.LeftEdge;
            case PlasmaCore.Types.RightEdge:
                return Qt.RightEdge;
            case PlasmaCore.Types.TopEdge:
                return Qt.TopEdge;
            default:
                return Qt.BottomEdge;
            }
        }

        Repeater {
            model: Activities.ActivityModel {
                id: activityModel
                shownStates: "Running"
            }
            delegate: PlasmaComponents.TabButton {
                id: tab
                checked: model.current
                text: model.name
                onClicked: {
                    activityModel.setCurrentActivity(model.id, function() {});
                }
                Component.onCompleted: {
                    if(model.current) {
                        tabBar.currentTab = tab;
                    }
                }
                onCheckedChanged: {
                    if(model.current) {
                        tabBar.currentTab = tab;
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        plasmoid.removeAction("configure");
    }
}
