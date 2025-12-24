/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents // FIXME: PC3 Tabbar only has top and bottom tab positions, not left and right

import org.kde.activities as Activities
import org.kde.plasma.activityswitcher as ActivitySwitcher
import org.kde.kcmutils // KCMLauncher
import org.kde.config // KAuthorized

PlasmoidItem {
    Layout.minimumWidth: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? tabBar.implicitHeight : tabBar.implicitWidth
    Layout.minimumHeight: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? tabBar.implicitWidth : tabBar.implicitHeight

    Plasmoid.constraintHints: Plasmoid.CanFillArea
    preferredRepresentation: fullRepresentation

    PlasmaComponents.TabBar {
        id: tabBar

        anchors.centerIn: parent
        rotation: Plasmoid.formFactor === PlasmaCore.Types.Vertical ?  -90 : 0
        transformOrigin: Item.Center
        width: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? parent.height : parent.width
        height: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? parent.width : parent.height


        position: {
            switch (Plasmoid.location) {
            case PlasmaCore.Types.LeftEdge:
            case PlasmaCore.Types.TopEdge:
                return PlasmaComponents.TabBar.Header;
            default:
                return PlasmaComponents.TabBar.Footer;
            }
        }

        Repeater {
            model: Activities.ActivityModel {
                id: activityModel
            }
            delegate: PlasmaComponents.TabButton {
                id: tab

                required property bool current
                required property string name
                required property string id

                checked: current
                text: name
                activeFocusOnTab: true
                width: implicitWidth

                Keys.onPressed: event => {
                    switch (event.key) {
                    case Qt.Key_Space:
                    case Qt.Key_Enter:
                    case Qt.Key_Return:
                    case Qt.Key_Select:
                        activityModel.setCurrentActivity(id, function() {});
                        event.accepted = true;
                        break;
                    }
                }
                Accessible.checked: current
                Accessible.name: name
                Accessible.description: i18n("Switch to activity %1", name)
                Accessible.role: Accessible.Button

                onClicked: {
                    activityModel.setCurrentActivity(id, function() {});
                }

                onCheckedChanged: {
                    if(current) {
                        if (tabBar.activeFocus) {
                            forceActiveFocus();
                        }
                    }
                }
            }
        }
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: i18nc("@action:inmenu widget context menu", "Show Activity Manager")
            icon.name: "activities"
            onTriggered: ActivitySwitcher.Backend.toggleActivityManager()
        }
    ]

    PlasmaCore.Action {
        id: configureAction
        text: i18nc("@action:inmenu", "&Configure Activities…")
        icon.name: "configure"
        visible: KAuthorized.authorizeControlModule("kcm_activities")
        onTriggered: KCMLauncher.openSystemSettings("kcm_activities")
    }

    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction);
    }
}
