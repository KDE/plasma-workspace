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

import org.kde.kcoreaddons 1.0 as KCoreAddons
import org.kde.kquickcontrolsaddons 2.0 as KQCAddons

import org.kde.notificationmanager 1.0 as NotificationManager

import "global"

Item {
    id: root 

    Plasmoid.status: historyModel.activeJobsCount > 0
                     || historyModel.unreadNotificationsCount > 0
                     || Globals.inhibited ? PlasmaCore.Types.ActiveStatus
                                          : PlasmaCore.Types.PassiveStatus

    Plasmoid.toolTipSubText: {
        var lines = [];

        if (historyModel.activeJobsCount > 0) {
            lines.push(i18np("%1 running job", "%1 running jobs", historyModel.activeJobsCount));
        }

        // Any notification that is newer than "lastRead" is "unread"
        // since it doesn't know the popup is on screen which makes the user see it
        var actualUnread = historyModel.unreadNotificationsCount - Globals.popupNotificationsModel.activeNotificationsCount;
        if (actualUnread > 0) {
            lines.push(i18np("%1 unread notification", "%1 unread notifications", actualUnread));
        }

        if (Globals.inhibited) {
            var inhibitedUntil = notificationSettings.notificationsInhibitedUntil
            var inhibitedUntilValid = !isNaN(inhibitedUntil.getTime());

            // TODO check app inhibition, too
            if (inhibitedUntilValid) {
                lines.push(i18n("Do not disturb until %1",
                             KCoreAddons.Format.formatRelativeDateTime(inhibitedUntil, Locale.ShortFormat)));
            }
        } else if (lines.length === 0) {
            lines.push(i18n("No unread notifications"));
        }

        return lines.join("\n");
    }

    Plasmoid.switchWidth: units.gridUnit * 14
    // This is to let the plasmoid expand in a vertical panel for a "sidebar" notification panel
    // The CompactRepresentation size is limited to not have the notification icon grow gigantic
    // but it should still switch over to full rep once there's enough width (disregarding the limited height)
    Plasmoid.switchHeight: plasmoid.formFactor === PlasmaCore.Types.Vertical ? 1 : units.gridUnit * 10

    Plasmoid.onExpandedChanged: {
        if (!plasmoid.expanded) {
            // FIXME Qt.callLater because system tray gets confused when an applet becomes passive when clicking to hide it
            Qt.callLater(function() {
                historyModel.lastRead = undefined; // reset to now
                historyModel.collapseAllGroups();
            });
        }
    }

    Plasmoid.compactRepresentation: CompactRepresentation {
        activeCount: Globals.popupNotificationsModel.activeNotificationsCount
        unreadCount: Math.min(99, historyModel.unreadNotificationsCount)

        jobsCount: historyModel.activeJobsCount
        jobsPercentage: historyModel.jobsPercentage

        inhibited: Globals.inhibited
    }

    Plasmoid.fullRepresentation: FullRepresentation {

    }

    NotificationManager.Settings {
        id: notificationSettings
    }

    NotificationManager.Notifications {
        id: historyModel
        showExpired: true
        showDismissed: true
        showJobs: notificationSettings.jobsInNotifications
        sortMode: NotificationManager.Notifications.SortByTypeAndUrgency
        groupMode: NotificationManager.Notifications.GroupApplicationsFlat
        groupLimit: 2
        expandUnread: true
        blacklistedDesktopEntries: notificationSettings.historyBlacklistedApplications
        blacklistedNotifyRcNames: notificationSettings.historyBlacklistedServices
        urgencies: {
            var urgencies = NotificationManager.Notifications.CriticalUrgency
                          | NotificationManager.Notifications.NormalUrgency;
            if (notificationSettings.lowPriorityHistory) {
                urgencies |= NotificationManager.Notifications.LowUrgency;
            }
            return urgencies;
        }
    }

    function action_clearHistory() {
        historyModel.clear(NotificationManager.Notifications.ClearExpired);
        if (historyModel.count === 0) {
            plasmoid.expanded = false;
        }
    }

    function action_openKcm() {
        KQCAddons.KCMShell.open("kcm_notifications");
    }

    Component.onCompleted: {
        Globals.adopt(plasmoid);

        plasmoid.setAction("clearHistory", i18n("Clear History"), "edit-clear-history");
        var clearAction = plasmoid.action("clearHistory");
        clearAction.visible = Qt.binding(function() {
            return historyModel.expiredNotificationsCount > 0;
        });

        // FIXME only while Multi-page KCMs are broken when embedded in plasmoid config
        plasmoid.setAction("openKcm", i18n("&Configure Event Notifications and Actions..."), "preferences-desktop-notification-bell");
        plasmoid.action("openKcm").visible = (KQCAddons.KCMShell.authorize("kcm_notifications.desktop").length > 0);
    }
}
