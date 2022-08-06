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
import org.kde.kquickcontrolsaddons 2.1 // For KCMShell

Item {
    Layout.minimumWidth: tabBar.implicitWidth
    Layout.minimumHeight: tabBar.implicitHeight

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation

    function action_activitieskcm() {
        KCMShell.openSystemSettings("kcm_activities");
    }

    PlasmaComponents.TabBar {
        id: tabBar

        anchors.fill: parent

        tabPosition: {
            switch (Plasmoid.location) {
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
                activeFocusOnTab: true

                Keys.onPressed: {
                    switch (event.key) {
                    case Qt.Key_Space:
                    case Qt.Key_Enter:
                    case Qt.Key_Return:
                    case Qt.Key_Select:
                        activityModel.setCurrentActivity(model.id, function() {});
                        event.accepted = true;
                        break;
                    }
                }
                Accessible.checked: model.current
                Accessible.name: model.name
                Accessible.description: i18n("Switch to activity %1", model.name)
                Accessible.role: Accessible.Button

                onClicked: {
                    activityModel.setCurrentActivity(model.id, function() {});
                }

                onCheckedChanged: {
                    if(model.current) {
                        tabBar.currentTab = tab;
                        if (tabBar.activeFocus) {
                            forceActiveFocus();
                        }
                    }
                }

                Component.onCompleted: {
                    if(model.current) {
                        tabBar.currentTab = tab;
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        Plasmoid.removeAction("configure");

        if (KCMShell.authorize("kcm_activities.desktop").length > 0) {
            Plasmoid.setAction("activitieskcm", i18nc("@action:inmenu", "&Configure Activities…"), "configure");
        }
    }
}
