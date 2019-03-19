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

QtObject {
    id: popupHandler

    // FIXME figure out which plasmoid to use (prefer one in panel, prefer primary screen, etc etc)
    property QtObject plasmoid: {
        return plasmoids[plasmoids.length - 1];

    }

    property var plasmoids: []

    // TODO configurable
    property int popupLocation: {
        if (!plasmoid) {
            return 0;
        }

        var alignment = 0;
        if (plasmoid.location === PlasmaCore.Types.LeftEdge) {
            alignment |= Qt.AlignLeft;
        } else {
            alignment |= Qt.AlignRight;
        }
        if (plasmoid.location === PlasmaCore.Types.TopEdge) {
            alignment |= Qt.AlignTop;
        } else {
            alignment |= Qt.AlignBottom;
        }
        return alignment;
    }

    // The raw width of the popup's content item, the Dialog itself adds some margins
    property int popupWidth: units.gridUnit * 18
    property int popupEdgeDistance: units.largeSpacing
    property int popupSpacing: units.largeSpacing

    // How much vertical screen real estate the notification popups may consume
    readonly property real popupMaximumScreenFill: 0.67

    property var screenRect: plasmoid.availableScreenRect

    onPopupLocationChanged: Qt.callLater(positionPopups)
    onScreenRectChanged: Qt.callLater(positionPopups)

    property QtObject popupNotificationsModel: NotificationManager.Notifications {
        // TODO make these configurable
        limit: Math.ceil(popupHandler.screenRect.height / (theme.mSize(theme.defaultFont).height * 4))
        showExpired: false
        showDismissed: false
        showJobs: true
        urgencies: NotificationManager.Notifications.NormalUrgency | NotificationManager.Notifications.CriticalUrgency
    }

    function adopt(plasmoid) {
        var newPlasmoids = plasmoids;
        newPlasmoids.push(plasmoid);
        popupHandler.plasmoids = newPlasmoids;
    }

    function positionPopups() {
        // TODO clean this up a little, also, would be lovely if we could mapToGlobal the applet and then
        // position the popups closed to it, e.g. if notification applet is on the left in the panel or sth like that?
        var rect = screenRect;
        if (!rect || rect.width <= 0 || rect.height <= 0) {
            console.warn("Cannot position popups on screen of size", JSON.stringify(screenRect));
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
        } else if (popupLocation & Qt.AlignHCenter) {
            x += (screenRect.width - popupWidth) / 2;
        } else if (popupLocation & Qt.AlignRight) {
            x += screenRect.width - popupEdgeDistance - popupWidth;
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

            // don't let notifications take more than 3/4 of the screen
            var visible = true;
            if (i > 0) { // however always show at least one popup
                if (popupLocation & Qt.AlignTop) {
                    visible = (popup.y + popup.height < screenRect.y + (screenRect.height * popupMaximumScreenFill));
                } else {
                    visible = (popup.y > screenRect.y + (screenRect.height * (1 - popupMaximumScreenFill)));
                }
            }

            // TODO hide popups when systray or panel controller is open or something
            /*popup.visible = Qt.binding(function() {
                return visible && !plasmoid.expanded;
            });*/
            popup.visible = visible;
        }
    }

    property Instantiator popupInstantiator: Instantiator {
        model: popupNotificationsModel
        delegate: NotificationPopup {
            // TODO defaultTimeout configurable

            notificationType: model.type

            applicationName: model.applicationName
            applicatonIconSource: model.applicationIconName

            time: model.updated || model.created

            configurable: model.configurable
            // For running jobs instead of offering a "close" button that might lead the user to
            // think that will cancel the job, we offer a "dismiss" button that hides it in the history
            dismissable: model.type === NotificationManager.Notifications.JobType
                && model.jobState !== NotificationManager.Notifications.JobStateStopped
            closable: model.type === NotificationManager.Notifications.NotificationType
                || model.jobState === NotificationManager.Notifications.JobStateStopped

            summary: model.summary
            body: model.body || "" // TODO
            icon: model.image || model.iconName
            hasDefaultAction: model.hasDefaultAction || false
            timeout: model.timeout
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

            onExpired: popupNotificationsModel.expire(popupNotificationsModel.makeModelIndex(index))
            onCloseClicked: popupNotificationsModel.close(popupNotificationsModel.makeModelIndex(index))
            onDismissClicked: popupNotificationsModel.dismiss(popupNotificationsModel.makeModelIndex(index))
            onConfigureClicked: popupNotificationsModel.configure(popupNotificationsModel.makeModelIndex(index))
            onDefaultActionInvoked: popupNotificationsModel.invokeDefaultAction(popupNotificationsModel.makeModelIndex(index))
            onActionInvoked: popupNotificationsModel.invokeAction(popupNotificationsModel.makeModelIndex(index), actionName)

            onSuspendJobClicked: popupNotificationsModel.suspendJob(popupNotificationsModel.makeModelIndex(index))
            onResumeJobClicked: popupNotificationsModel.resumeJob(popupNotificationsModel.makeModelIndex(index))
            onKillJobClicked: popupNotificationsModel.killJob(popupNotificationsModel.makeModelIndex(index))

            onHeightChanged: Qt.callLater(positionPopups)
            onWidthChanged: Qt.callLater(positionPopups)
        }
        onObjectAdded: {
            // also needed for it to correctly layout its contents
            object.visible = true;
            Qt.callLater(positionPopups)
        }
        onObjectRemoved: Qt.callLater(positionPopups)
    }
}
