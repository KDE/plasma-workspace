/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.8

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kquickcontrolsaddons 2.0

import org.kde.notificationmanager 1.0 as NotificationManager

import "popups" as Popups

Item {
    id: root 

    Plasmoid.status: PlasmaCore.Types.PassiveStatus

    Plasmoid.toolTipSubText: {
        var lines = [];

        if (historyModel.activeJobsCount > 0) {
            lines.push(i18np("%1 running job", "%1 running jobs", historyModel.activeJobsCount));
        }

        if (historyModel.unreadNotificationsCount > 0) {
            lines.push(i18np("%1 unread notification", "%1 unread notifications", historyModel.unreadNotificationsCount));
        }

        if (notificationSettings.notificationsInhibited) {
            lines.push(i18n("Do not disturb mode enabled"));
        } else if (lines.length === 0) {
            lines.push("No unread notificatons");
        }

        return lines.join("\n");
    }

    Plasmoid.switchWidth: units.gridUnit * 14
    Plasmoid.switchHeight: units.gridUnit * 10

    Plasmoid.onExpandedChanged: {
        historyModel.lastRead = undefined; // reset to now
    }

    Plasmoid.compactRepresentation: CompactRepresentation {
        activeCount: Popups.PopupHandler.popupNotificationsModel.activeNotificationsCount
        unreadCount: historyModel.unreadNotificationsCount

        jobsCount: historyModel.activeJobsCount
        jobsPercentage: historyModel.jobsPercentage

        inhibited: notificationSettings.notificationsInhibited
    }

    Plasmoid.fullRepresentation: FullRepresentation {

    }

    // Delay hiding the applet again so the user can see the unread count briefly before it goes away
    Timer {
        id: updateStatusTimer
        readonly property int targetStatus: historyModel.activeJobsCount > 0
                                            || Popups.PopupHandler.popupNotificationsModel.activeNotificationsCount > 0
                                            || notificationSettings.notificationsInhibited ? PlasmaCore.Types.ActiveStatus
                                                                                           : PlasmaCore.Types.PassiveStatus
        interval: 2000

        onTargetStatusChanged: {
            if (targetStatus === PlasmaCore.Types.ActiveStatus) {
                // become active right away
                updateStatusTimer.stop();
                updateStatusTimer.triggered();
            } else {
                updateStatusTimer.start();
            }
        }

        onTriggered: plasmoid.status = targetStatus;
        Component.onCompleted: triggered() // set correct status initially
    }

    NotificationManager.Settings {
        id: notificationSettings
    }

    NotificationManager.Notifications {
        id: historyModel
        showExpired: true
        showDismissed: true
        showJobs: notificationSettings.jobsInNotifications
        sortMode: NotificationManager.Notifications.SortByDate
        groupMode: NotificationManager.Notifications.GroupApplicationsFlat
        blacklistedDesktopEntries: notificationSettings.historyBlacklistedApplications
        blacklistedNotifyRcNames: notificationSettings.historyBlacklistedServices
    }

    Component.onCompleted: {
        Popups.PopupHandler.adopt(plasmoid)
    }
}
