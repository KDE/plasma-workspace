/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
