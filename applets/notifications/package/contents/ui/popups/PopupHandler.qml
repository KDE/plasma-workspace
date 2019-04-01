/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

pragma Singleton
import QtQuick 2.8
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.kquickcontrolsaddons 2.0

import org.kde.notificationmanager 1.0 as NotificationManager

import ".."

QtObject {
    id: popupHandler

    // Some parts of the code rely on plasmoid.nativeInterface and since we're in a singleton here
    // this is named "plasmoid", TODO fix?
    property QtObject plasmoid: plasmoids[0]
    // all notification plasmoids
    property var plasmoids: []

    // This heuristic tries to find a suitable plasmoid to follow when placing popups
    function plasmoidScore(plasmoid) {
        if (!plasmoid) {
            return 0;
        }

        var score = 0;

        // Prefer plasmoids in a panel, prefer horizontal panels over vertical ones
        if (plasmoid.location === PlasmaCore.Types.LeftEdge
                || plasmoid.location === PlasmaCore.Types.RightEdge) {
            score += 1;
        } else if (plasmoid.location === PlasmaCore.Types.TopEdge
                   || plasmoid.location === PlasmaCore.Types.BottomEdge) {
            score += 2;
        }

        // Prefer iconified plasmoids
        if (!plasmoid.expanded) {
            ++score;
        }

        // Prefer plasmoids on primary screen
        if (plasmoid.nativeInterface && plasmoid.nativeInterface.isPrimaryScreen(plasmoid.screenGeometry)) {
            ++score;
        }

        return score;
    }

    property int popupLocation: {
        switch (notificationSettings.popupPosition) {
        // Auto-determine location based on plasmoid location
        case NotificationManager.Settings.NearWidget:
            if (!plasmoid) {
                return Qt.AlignBottom | Qt.AlignRight; // just in case
            }

            var alignment = 0;
            if (plasmoid.location === PlasmaCore.Types.LeftEdge) {
                alignment |= Qt.AlignLeft;
            } else {
                // would be nice to do plasmoid.compactRepresentationItem.mapToItem(null) and then
                // position the popups depending on the relative position within the panel
                alignment |= Qt.AlignRight;
            }
            if (plasmoid.location === PlasmaCore.Types.TopEdge) {
                alignment |= Qt.AlignTop;
            } else {
                alignment |= Qt.AlignBottom;
            }
            return alignment;

        case NotificationManager.Settings.TopLeft:
            return Qt.AlignTop | Qt.AlignLeft;
        case NotificationManager.Settings.TopCenter:
            return Qt.AlignTop | Qt.AlignHCenter;
        case NotificationManager.Settings.TopRight:
            return Qt.AlignTop | Qt.AlignRight;
        case NotificationManager.Settings.BottomLeft:
            return Qt.AlignBottom | Qt.AlignLeft;
        case NotificationManager.Settings.BottomCenter:
            return Qt.AlignBottom | Qt.AlignHCenter;
        case NotificationManager.Settings.BottomRight:
            return Qt.AlignBottom | Qt.AlignRight;
        }
    }

    // The raw width of the popup's content item, the Dialog itself adds some margins
    property int popupWidth: units.gridUnit * 18
    property int popupEdgeDistance: units.largeSpacing
    property int popupSpacing: units.largeSpacing

    // How much vertical screen real estate the notification popups may consume
    readonly property real popupMaximumScreenFill: 0.75

    property var screenRect: plasmoid.availableScreenRect

    onPopupLocationChanged: Qt.callLater(positionPopups)
    onScreenRectChanged: Qt.callLater(positionPopups)

    function adopt(plasmoid) {
        var newPlasmoids = plasmoids;
        newPlasmoids.push(plasmoid);
        newPlasmoids.sort(function (a, b) {
            var scoreA = plasmoidScore(a);
            var scoreB = plasmoidScore(b);
            // Sort descending by score
            if (scoreA < scoreB) {
                return 1;
            } else if (scoreA > scoreB) {
                return -1;
            } else {
                return 0;
            }
        });

        popupHandler.plasmoids = newPlasmoids;
    }

    function positionPopups() {
        var rect = screenRect;
        if (!rect || rect.width <= 0 || rect.height <= 0) {
            return;
        }

        var y = screenRect.y;
        if (popupLocation & Qt.AlignBottom) {
            y += screenRect.height;
        } else {
            y += popupEdgeDistance;
        }

        var x = screenRect.x;
        if (popupLocation & Qt.AlignLeft) {
            x += popupEdgeDistance;
        }

        for (var i = 0; i < popupInstantiator.count; ++i) {
            var popup = popupInstantiator.objectAt(i);

            if (popupLocation & Qt.AlignHCenter) {
                popup.x = x + (screenRect.width - popup.width) / 2;
            } else if (popupLocation & Qt.AlignRight) {
                popup.x = screenRect.width - popupEdgeDistance - popup.width;
            } else {
                popup.x = x;
            }

            var delta = popupSpacing + popup.height;

            if (popupLocation & Qt.AlignTop) {
                popup.y = y;
                y += delta;
            } else {
                y -= delta;
                popup.y = y;
            }

            // don't let notifications take more than popupMaximumScreenFill of the screen
            var visible = true;
            if (i > 0) { // however always show at least one popup
                if (popupLocation & Qt.AlignTop) {
                    visible = (popup.y + popup.height < screenRect.y + (screenRect.height * popupMaximumScreenFill));
                } else {
                    visible = (popup.y > screenRect.y + (screenRect.height * (1 - popupMaximumScreenFill)));
                }
            }

            // TODO would be nice to hide popups when systray or panel controller is open
            popup.visible = visible;
        }
    }

    property QtObject popupNotificationsModel: NotificationManager.Notifications {
        limit: Math.ceil(popupHandler.screenRect.height / (theme.mSize(theme.defaultFont).height * 4))
        showExpired: false
        showDismissed: false
        blacklistedDesktopEntries: notificationSettings.popupBlacklistedApplications
        blacklistedNotifyRcNames: notificationSettings.popupBlacklistedServices
        showJobs: notificationSettings.jobsInNotifications
        sortMode: NotificationManager.Notifications.SortByTypeAndUrgency
        groupMode: NotificationManager.Notifications.GroupDisabled
        urgencies: {
            var urgencies = NotificationManager.Notifications.NormalUrgency | NotificationManager.Notifications.CriticalUrgency;
            if (notificationSettings.lowPriorityPopups) {
                urgencies |=NotificationManager.Notifications.LowUrgency;
            }
            return urgencies;
        }
    }

    property QtObject notificationSettings: NotificationManager.Settings { }

    property Instantiator popupInstantiator: Instantiator {
        model: popupNotificationsModel
        delegate: NotificationPopup {
            notificationType: model.type

            applicationName: model.applicationName
            applicatonIconSource: model.applicationIconName
            deviceName: model.deviceName || ""

            time: model.updated || model.created

            configurable: model.configurable
            // For running jobs instead of offering a "close" button that might lead the user to
            // think that will cancel the job, we offer a "dismiss" button that hides it in the history
            dismissable: model.type === NotificationManager.Notifications.JobType
                && model.jobState !== NotificationManager.Notifications.JobStateStopped
            // TODO would be nice to be able to "pin" jobs when they autohide
                && notificationSettings.permanentJobPopups
            closable: model.type === NotificationManager.Notifications.NotificationType
                || model.jobState === NotificationManager.Notifications.JobStateStopped

            summary: model.summary
            body: model.body || ""
            icon: model.image || model.iconName
            hasDefaultAction: model.hasDefaultAction || false
            timeout: model.timeout
            defaultTimeout: notificationSettings.popupTimeout
            // When configured to not keep jobs open permanently, we autodismiss them after the standard timeout
            dismissTimeout: !notificationSettings.permanentJobPopups
                            && model.type === NotificationManager.Notifications.JobType
                            && model.jobState !== NotificationManager.Notifications.JobStateStopped
                            ? defaultTimeout : 0

            urls: model.urls || []
            urgency: model.urgency

            jobState: model.jobState || 0
            percentage: model.percentage || 0
            error: model.error || 0
            errorText: model.errorText || ""
            suspendable: !!model.suspendable
            killable: !!model.killable
            jobDetails: model.jobDetails || null

            configureActionLabel: model.configureActionLabel || ""
            actionNames: model.actionNames
            actionLabels: model.actionLabels

            onExpired: popupNotificationsModel.expire(popupNotificationsModel.index(index, 0))
            onCloseClicked: popupNotificationsModel.close(popupNotificationsModel.index(index, 0))
            onDismissClicked: model.dismissed = true
            onConfigureClicked: popupNotificationsModel.configure(popupNotificationsModel.index(index, 0))
            onDefaultActionInvoked: {
                popupNotificationsModel.invokeDefaultAction(popupNotificationsModel.index(index, 0))
                popupNotificationsModel.close(popupNotificationsModel.index(index, 0))
            }
            onActionInvoked: {
                popupNotificationsModel.invokeAction(popupNotificationsModel.index(index, 0), actionName)
                popupNotificationsModel.close(popupNotificationsModel.index(index, 0))
            }
            onOpenUrl: {
                Qt.openUrlExternally(url);
                popupNotificationsModel.close(popupNotificationsModel.index(index, 0))
            }
            onFileActionInvoked: popupNotificationsModel.close(popupNotificationsModel.index(index, 0))

            onSuspendJobClicked: popupNotificationsModel.suspendJob(popupNotificationsModel.index(index, 0))
            onResumeJobClicked: popupNotificationsModel.resumeJob(popupNotificationsModel.index(index, 0))
            onKillJobClicked: popupNotificationsModel.killJob(popupNotificationsModel.index(index, 0))

            onHeightChanged: Qt.callLater(positionPopups)
            onWidthChanged: Qt.callLater(positionPopups)

            Component.onCompleted: {
                // Register apps that were seen spawning a popup so they can be configured later
                // Apps with notifyrc can already be configured anyway
                if (model.desktopEntry && !model.notifyRcName) {
                    notificationSettings.registerKnownApplication(model.desktopEntry);
                    notificationSettings.save();
                }
            }
        }
        onObjectAdded: {
            // also needed for it to correctly layout its contents
            object.visible = true;
            Qt.callLater(positionPopups)
        }
        onObjectRemoved: Qt.callLater(positionPopups)
    }
}
