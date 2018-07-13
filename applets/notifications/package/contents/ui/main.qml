/***************************************************************************
 *   Copyright 2011 Davide Bettio <davide.bettio@kdemail.net>              *
 *   Copyright 2011 Marco Martin <mart@kde.org>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.plasma.private.notifications 1.0

import "uiproperties.js" as UiProperties

MouseEventListener {
    id: notificationsApplet
    //width: units.gridUnit.width * 10
    //height: units.gridUnit.width * 15

    //Layout.minimumWidth: mainScrollArea.implicitWidth
    //Layout.minimumHeight: mainScrollArea.implicitHeight
    Layout.minimumWidth: 256 // FIXME: use above
    Layout.minimumHeight: 256

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property int layoutSpacing: UiProperties.layoutSpacing

    property real globalProgress: 0

    property Item notifications: historyList.headerItem ? historyList.headerItem.notifications : null
    property Item jobs: historyList.headerItem ? historyList.headerItem.jobs : null
    
    //notifications + jobs
    property int activeItemsCount: (notifications ? notifications.count : 0) + (jobs ? jobs.count : 0)
    property int totalCount: activeItemsCount + (notifications ? notifications.historyCount : 0)

    Plasmoid.switchWidth: units.gridUnit * 20
    Plasmoid.switchHeight: units.gridUnit * 30

    Plasmoid.status: activeItemsCount > 0 ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus

    Plasmoid.toolTipSubText: {
        if (activeItemsCount == 0) {
            return i18n("No notifications or jobs")
        } else if (!notifications || !notifications.count) {
            return i18np("%1 running job", "%1 running jobs", jobs.count)
        } else if (!jobs || !jobs.count) {
            return i18np("%1 notification", "%1 notifications", notifications.count)
        } else {
            return i18np("%1 running job", "%1 running jobs", jobs.count) + "\n" + i18np("%1 notification", "%1 notifications", notifications.count)
        }
    }

    Plasmoid.compactRepresentation: NotificationIcon { }

    // Always scroll to the top when opening as that's where the important stuff goes on
    Plasmoid.onExpandedChanged: {
        if (Plasmoid.expanded) {
            // contentY doesn't really work with ListView (creates and destroys delegates on demand and positions them randomly)
            // so first use its "move to the top" method and then move it further up to reveal all of its "header" contents
            historyList.positionViewAtBeginning();
            historyList.contentY = historyList.originY;
        }
    }

    hoverEnabled: !UiProperties.touchInput

    onActiveItemsCountChanged: {
        if (!activeItemsCount) {
            plasmoid.expanded = false;
        }
    }

    PlasmaExtras.Heading {
        width: parent.width
        level: 3
        opacity: 0.6
        visible: notificationsApplet.totalCount == 0
        text: i18n("No new notifications.")
    }

    PlasmaExtras.ScrollArea {
        id: mainScrollArea
        anchors.fill: parent

        // HACK The history of notifications can become quite large. In order to avoid a memory leak
        // show them in a ListView which creates delegate instances only on demand.
        // The ListView's header functionality is abused to provide the jobs and regular notifications
        // which are few and might store some state inside the delegate (e.g. expanded state) and
        // thus are created all at once by a Repeater.
        ListView {
            id: historyList

            // The history stuff is quite entangled with regular notifications, so
            // model and delegate are set by Bindings {} inside Notifications.qml

            header: Column {
                property alias jobs: jobsLoader.item
                property alias notifications: notificationsLoader.item

                width: historyList.width

                Loader {
                    id: jobsLoader
                    width: parent.width
                    source: "Jobs.qml"
                    active: plasmoid.configuration.showJobs
                }

                Loader {
                    id: notificationsLoader
                    width: parent.width
                    source: "Notifications.qml"
                    active: plasmoid.configuration.showNotifications
                }
            }
        }
    }

    function action_clearNotifications() {
        notifications.clearNotifications();
        notifications.clearHistory();
    }

    function action_notificationskcm() {
        KCMShell.open("kcmnotify");
    }

    Component.onCompleted: {
        plasmoid.setAction("clearNotifications", i18n("Clear Notifications"), "edit-clear-history")
        var clearAction = plasmoid.action("clearNotifications");
        clearAction.visible = Qt.binding(function() {
            return notificationsApplet.notifications && (notificationsApplet.notifications.count > 0 || notificationsApplet.notifications.historyCount > 0);
        })

        if (KCMShell.authorize("kcmnotify.desktop").length > 0) {
            plasmoid.setAction("notificationskcm", i18n("&Configure Event Notifications and Actions..."), "preferences-desktop-notification")
        }
    }
}
