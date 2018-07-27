/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
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
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.kquickcontrolsaddons 2.0

import org.kde.notificationmanager 1.0 as NotificationManager

Item {
    id: root

    // TODO status depending on unread notifications
    //Plasmoid.status:

    // TODO tooltip depending on unread and persistent notifications, running jobs, etc?
    //Plasmoid.toolTipSubText:

    Plasmoid.compactRepresentation: CompactRepresentation {}

    Plasmoid.fullRepresentation: FullRepresentation {}

    Plasmoid.onLocationChanged: Qt.callLater(positionPopups)
    Plasmoid.onAvailableScreenRectChanged: Qt.callLater(positionPopups)

    // mapping of notificationId -> NotificationPopup instance
    property var popups: ({})

    // should we reuse popup instances?
    Component {
        id: popupComponent
        NotificationPopup {}
    }

    // this might need to go into C++ but I'd like to avoid the horror of the old implementation
    function positionPopups() {
        // TODO clean this up a little, also, would be lovely if we could mapToGlobal the applet and then
        // position the popups closed to it, e.g. if notification applet is on the left in the panel or sth like that?
        var popupSpacing = units.gridUnit;

        var y = plasmoid.availableScreenRect.y;
        if (plasmoid.location !== PlasmaCore.Types.TopEdge) {
            y += plasmoid.availableScreenRect.height;
        }

        var x = plasmoid.availableScreenRect.x;
        if (plasmoid.location !== PlasmaCore.Types.LeftEdge) {
            x += plasmoid.availableScreenRect.width - popupSpacing;
        } else {
            x += popupSpacing;
        }

        // FIXME don't assume order of items in object or that higher id == newer notification
        for (var id in popups) {
            var popup = popups[id];

            if (plasmoid.location !== PlasmaCore.Types.LeftEdge) {
                popup.x = x - popup.width;
            } else {
                popup.x = x;
            }

            if (popup.visible) {
                var delta = popupSpacing + popup.height;

                if (plasmoid.location === PlasmaCore.Types.TopEdge) {
                    popup.y = y;
                    y += delta;
                } else {
                    y -= delta;
                    popup.y = y;
                }
            }
        }
    }

    function closePopup(notificationId) {
        var popup = popups[notificationId];
        if (popup) {
            popup.visible = false;
            Qt.callLater(positionPopups);
            popup.destroy();
            delete popups[notificationId];
        }
    }

    // Popup handling, pretty crude
    Connections {
        target: NotificationManager.NotificationModel
        onRowsInserted: {
            var model = NotificationManager.NotificationModel;

            for (var i = first; i <= last; ++i) {
                var idx = model.index(i, 0, parent);

                var notificationId = model.data(idx, model.IdRole);

                var props = {
                    // can we use some kind of introspection for getting those?
                    // oooooor just pass a persistentmodelindex to the popup and have it figure it out?
                    notificationId: notificationId,
                    summary: model.data(idx, model.SummaryRole),
                    body: model.data(idx, model.BodyRole),
                    icon: model.data(idx, model.IconNameRole)
                          || model.data(idx, model.ImageRole),
                    hasDefaultAction: model.data(idx, model.HasDefaultActionRole),
                    timeout: model.data(idx, model.TimeoutRole)
                };

                var popup = popupComponent.createObject(root, props);
                // needs to be scoped or else "idx" is the last one in the loop by the time this callback is reached
                // TODO use "let" once QtQml supports ECMAScript 6 :)
                (function (notificationId) {
                    popup.closeClicked.connect(function() {
                        model.dismiss(notificationId);
                    });
                    popup.defaultActionInvoked.connect(function() {
                        model.invokeDefaultAction(notificationId);
                    });
                    popup.expired.connect(function() {
                        model.expire(notificationId);
                    });
                })(notificationId);

                popups[notificationId] = popup;
                Qt.callLater(positionPopups);
                // FIXME figure out positioning
                popup.visible = true;
            }
        }

        // the stuff below is also pretty crude. We also want grouping in the popup
        // so perhaps instead the popup should use a proxy model filtering by the ones in the respective
        // popup and if it goes empty (rows removed, marked as expired, etc) the popup closes
        onDataChanged: {
            var model = NotificationManager.NotificationModel;

            // check if maybe a notification got marked as expired
            if (roles.length > 0 && roles.indexOf(model.ExpiredRole) === -1) {
                return;
            }

            // FIXME iterate all indices
            var isExpired = model.data(topLeft, model.ExpiredRole);
            if (isExpired) {
                var notificationId = model.data(topLeft, model.IdRole);
                if (notificationId) {
                    closePopup(notificationId);
                }
            }
        }

        onRowsAboutToBeRemoved: {
            var model = NotificationManager.NotificationModel;

            for (var i = first; i <= last; ++i) {
                var idx = model.index(i, 0, parent);

                var notificationId = model.data(idx, model.IdRole);
                closePopup(notificationId);
            }
        }
    }

    function action_notificationskcm() {
        KCMShell.open("kcmnotify");
    }

    Component.onCompleted: {
        if (KCMShell.authorize("kcmnotify.desktop").length > 0) {
            plasmoid.setAction("notificationskcm", i18n("&Configure Event Notifications and Actions..."), "preferences-desktop-notification")
        }
    }
}
