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

// This singleton object contains stuff shared between all notification plasmoids, namely:
// - Popup creation and placement
// - Do not disturb mode
QtObject {
    id: globals

    // Listened to by "ago" label in NotificationHeader to update all of them in unison
    signal timeChanged

    property bool inhibited: false

    onInhibitedChanged: {
        var pa = pulseAudio.item;
        if (!pa) {
            return;
        }

        var stream = pa.notificationStream;
        if (!stream) {
            return;
        }

        if (inhibited) {
            // Only remember that we muted if previously not muted.
            if (!stream.muted) {
                notificationSettings.notificationSoundsInhibited = true;
                stream.mute();
            }
        } else {
            // Only unmute if we previously muted it.
            if (notificationSettings.notificationSoundsInhibited) {
                stream.unmute();
            }
            notificationSettings.notificationSoundsInhibited = false;
        }
        notificationSettings.save();
    }

    // Some parts of the code rely on plasmoid.nativeInterface and since we're in a singleton here
    // this is named "plasmoid"
    property QtObject plasmoid: plasmoids[0]

    // HACK When a plasmoid is destroyed, QML sets its value to "null" in the Array
    // so we then remove it so we have a working "plasmoid" again
    onPlasmoidChanged: {
        if (!plasmoid) {
            // this doesn't emit a change, only in ratePlasmoids() it will detect the change
            plasmoids.splice(0, 1); // remove first
            ratePlasmoids();
        }
    }

    // all notification plasmoids
    property var plasmoids: []

    property int popupLocation: {
        switch (notificationSettings.popupPosition) {
        // Auto-determine location based on plasmoid location
        case NotificationManager.Settings.CloseToWidget:
            if (!plasmoid) {
                return Qt.AlignBottom | Qt.AlignRight; // just in case
            }

            var alignment = 0;
            if (plasmoid.location === PlasmaCore.Types.LeftEdge) {
                alignment |= Qt.AlignLeft;
            } else if (plasmoid.location === PlasmaCore.Types.RightEdge) {
                alignment |= Qt.AlignRight;
            } else {
                // would be nice to do plasmoid.compactRepresentationItem.mapToItem(null) and then
                // position the popups depending on the relative position within the panel
                alignment |= Qt.application.layoutDirection === Qt.RightToLeft ? Qt.AlignLeft : Qt.AlignRight;
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
    property int popupEdgeDistance: units.largeSpacing * 2
    property int popupSpacing: units.largeSpacing

    // How much vertical screen real estate the notification popups may consume
    readonly property real popupMaximumScreenFill: 0.75

    onPopupLocationChanged: Qt.callLater(positionPopups)

    Component.onCompleted: checkInhibition()

    function adopt(plasmoid) {
        // this doesn't emit a change, only in ratePlasmoids() it will detect the change
        globals.plasmoids.push(plasmoid);
        ratePlasmoids();
    }

    // Sorts plasmoids based on a heuristic to find a suitable plasmoid to follow when placing popups
    function ratePlasmoids() {
        var plasmoidScore = function(plasmoid) {
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

        var newPlasmoids = plasmoids;
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
        globals.plasmoids = newPlasmoids;
    }

    function checkInhibition() {
        globals.inhibited = Qt.binding(function() {
            var inhibited = false;

            var inhibitedUntil = notificationSettings.notificationsInhibitedUntil;
            if (!isNaN(inhibitedUntil.getTime())) {
                inhibited |= (new Date().getTime() < inhibitedUntil.getTime());
            }

            if (notificationSettings.notificationsInhibitedByApplication) {
                inhibited |= true;
            }

            if (notificationSettings.inhibitNotificationsWhenScreensMirrored) {
                inhibited |= notificationSettings.screensMirrored;
            }

            return inhibited;
        });
    }

    function positionPopups() {
        if (!plasmoid) {
            return;
        }

        var screenRect = Qt.rect(plasmoid.screenGeometry.x + plasmoid.availableScreenRect.x,
                                 plasmoid.screenGeometry.y + plasmoid.availableScreenRect.y,
                                 plasmoid.availableScreenRect.width,
                                 plasmoid.availableScreenRect.height);
        if (screenRect.width <= 0 || screenRect.height <= 0) {
            return;
        }

        var y = screenRect.y;
        if (popupLocation & Qt.AlignBottom) {
            y += screenRect.height - popupEdgeDistance;
        } else {
            y += popupEdgeDistance;
        }

        var x = screenRect.x;
        if (popupLocation & Qt.AlignLeft) {
            x += popupEdgeDistance;
        }

        for (var i = 0; i < popupInstantiator.count; ++i) {
            let popup = popupInstantiator.objectAt(i);
            // Popup width is fixed, so don't rely on the actual window size
            var popupEffectiveWidth = popupWidth + popup.margins.left + popup.margins.right;

            if (popupLocation & Qt.AlignHCenter) {
                popup.x = x + (screenRect.width - popupEffectiveWidth) / 2;
            } else if (popupLocation & Qt.AlignRight) {
                popup.x = x + screenRect.width - popupEdgeDistance - popupEffectiveWidth;
            } else {
                popup.x = x;
            }

            if (popupLocation & Qt.AlignTop) {
                popup.y = y;
                // If the popup isn't ready yet, ignore its occupied space for now.
                // We'll reposition everything in onHeightChanged eventually.
                y += popup.height + (popup.height > 0 ? popupSpacing : 0);
            } else {
                y -= popup.height;
                popup.y = y;
                if (popup.height > 0) {
                    y -= popupSpacing;
                }
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

            popup.visible = Qt.binding(function() {
                const dialog = plasmoid.nativeInterface.focussedPlasmaDialog;
                if (dialog && dialog.visible && dialog !== popup) {
                    // If the notification obscures any other Plasma dialog, hide it
                    // No rect intersects in JS...
                    if (dialog.x < popup.x + popup.width && popup.x < dialog.x + dialog.width && dialog.y < popup.y + popup.height && popup.y < dialog.y + dialog.height) {
                        return false;
                    }
                }

                return visible;
            });
        }
    }

    property QtObject popupNotificationsModel: NotificationManager.Notifications {
        limit: plasmoid ? (Math.ceil(plasmoid.availableScreenRect.height / (theme.mSize(theme.defaultFont).height * 4))) : 0
        showExpired: false
        showDismissed: false
        blacklistedDesktopEntries: notificationSettings.popupBlacklistedApplications
        blacklistedNotifyRcNames: notificationSettings.popupBlacklistedServices
        whitelistedDesktopEntries: globals.inhibited ? notificationSettings.doNotDisturbPopupWhitelistedApplications : []
        whitelistedNotifyRcNames: globals.inhibited ? notificationSettings.doNotDisturbPopupWhitelistedServices : []
        showJobs: notificationSettings.jobsInNotifications
        sortMode: NotificationManager.Notifications.SortByTypeAndUrgency
        groupMode: NotificationManager.Notifications.GroupDisabled
        urgencies: {
            var urgencies = 0;

            // Critical always except in do not disturb mode when disabled in settings
            if (!globals.inhibited || notificationSettings.criticalPopupsInDoNotDisturbMode) {
                urgencies |= NotificationManager.Notifications.CriticalUrgency;
            }

            // Normal only when not in do not disturb mode
            if (!globals.inhibited) {
                urgencies |= NotificationManager.Notifications.NormalUrgency;
            }

            // Low only when enabled in settings and not in do not disturb mode
            if (!globals.inhibited && notificationSettings.lowPriorityPopups) {
                urgencies |=NotificationManager.Notifications.LowUrgency;
            }

            return urgencies;
        }
    }

    property QtObject notificationSettings: NotificationManager.Settings {
        onNotificationsInhibitedUntilChanged: globals.checkInhibition()
    }

    // This periodically checks whether do not disturb mode timed out and updates the "minutes ago" labels
    property QtObject timeSource: PlasmaCore.DataSource {
        engine: "time"
        connectedSources: ["Local"]
        interval: 60000 // 1 min
        intervalAlignment: PlasmaCore.Types.AlignToMinute
        onDataChanged: {
            checkInhibition();
            globals.timeChanged();
        }
    }

    property Instantiator popupInstantiator: Instantiator {
        model: popupNotificationsModel
        delegate: NotificationPopup {
            // so Instantiator can access that after the model row is gone
            readonly property var notificationId: model.notificationId

            popupWidth: globals.popupWidth
            type: model.urgency === NotificationManager.Notifications.CriticalUrgency && notificationSettings.keepCriticalAlwaysOnTop
                  ? PlasmaCore.Dialog.CriticalNotification : PlasmaCore.Dialog.Notification

            notificationType: model.type

            applicationName: model.applicationName
            applicationIconSource: model.applicationIconName
            originName: model.originName || ""

            time: model.updated || model.created

            configurable: model.configurable
            // For running jobs instead of offering a "close" button that might lead the user to
            // think that will cancel the job, we offer a "dismiss" button that hides it in the history
            dismissable: model.type === NotificationManager.Notifications.JobType
                && model.jobState !== NotificationManager.Notifications.JobStateStopped
            // TODO would be nice to be able to "pin" jobs when they autohide
                && notificationSettings.permanentJobPopups
            closable: model.closable

            summary: model.summary
            body: model.body || ""
            icon: model.image || model.iconName
            hasDefaultAction: model.hasDefaultAction || false
            timeout: model.timeout
            // Increase default timeout for notifications with a URL so you have enough time
            // to interact with the thumbnail or bring the window to the front where you want to drag it into
            defaultTimeout: notificationSettings.popupTimeout + (model.urls && model.urls.length > 0 ? 5000 : 0)
            // When configured to not keep jobs open permanently, we autodismiss them after the standard timeout
            dismissTimeout: !notificationSettings.permanentJobPopups
                            && model.type === NotificationManager.Notifications.JobType
                            && model.jobState !== NotificationManager.Notifications.JobStateStopped
                            ? defaultTimeout : 0

            urls: model.urls || []
            urgency: model.urgency || NotificationManager.Notifications.NormalUrgency

            jobState: model.jobState || 0
            percentage: model.percentage || 0
            jobError: model.jobError || 0
            suspendable: !!model.suspendable
            killable: !!model.killable
            jobDetails: model.jobDetails || null

            configureActionLabel: model.configureActionLabel || ""
            actionNames: model.actionNames
            actionLabels: model.actionLabels

            onExpired: popupNotificationsModel.expire(popupNotificationsModel.index(index, 0))
            onHoverEntered: model.read = true
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

            // popup width is fixed
            onHeightChanged: positionPopups()

            Component.onCompleted: {
                // Register apps that were seen spawning a popup so they can be configured later
                // Apps with notifyrc can already be configured anyway
                if (model.type === NotificationManager.Notifications.NotificationType && model.desktopEntry && !model.notifyRcName) {
                    notificationSettings.registerKnownApplication(model.desktopEntry);
                    notificationSettings.save();
                }

                // Tell the model that we're handling the timeout now
                popupNotificationsModel.stopTimeout(popupNotificationsModel.index(index, 0));
            }
        }
        onObjectAdded: {
            positionPopups();
            object.visible = true;
        }
        onObjectRemoved: {
            var notificationId = object.notificationId
            // Popup might have been destroyed because of a filter change, tell the model to do the timeout work for us again
            // cannot use QModelIndex here as the model row is already gone
            popupNotificationsModel.startTimeout(notificationId);

            positionPopups();
        }
    }

    // TODO use pulseaudio-qt for this once it becomes a framework
    property QtObject pulseAudio: Loader {
        source: "PulseAudio.qml"
    }

    property Connections screenWatcher: Connections {
        target: plasmoid
        onAvailableScreenRectChanged: repositionTimer.start()
        onScreenGeometryChanged: repositionTimer.start()
    }

    // Normally popups are repositioned through Qt.callLater but in case of e.g. screen geometry changes we want to compress that
    property Timer repositionTimer: Timer {
        interval: 250
        onTriggered: positionPopups()
    }

    // Keeps the Inhibited property on DBus in sync with our inhibition handling
    property Binding serverInhibitedBinding: Binding {
        target: NotificationManager.Server
        property: "inhibited"
        value: globals.inhibited
    }
}
