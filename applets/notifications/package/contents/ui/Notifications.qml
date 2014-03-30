/*
 *   Copyright 2012 Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

import org.kde.plasma.private.notifications 1.0


Column {
    id: notificationsRoot
    anchors {
        left: parent.left
        right: parent.right
    }

    property QtObject notificationPopup
    property alias count: notificationsRepeater.count
    property var notificationStack

    Component.onCompleted: {
        notificationStack = new Array()
    }

    function addNotification(source, appIcon, image, appName, summary, body, isPersistent, expireTimeout, urgency, appRealName, configurable, actions) {
        // Do not show duplicated notifications
        for (var i = 0; i < notificationsModel.count; ++i) {
            if (notificationsModel.get(i).source == source &&
                notificationsModel.get(i).appName == appName &&
                notificationsModel.get(i).summary == summary &&
                notificationsModel.get(i).body == body) {
                return
            }
        }

        for (var i = 0; i < notificationsModel.count; ++i) {
            if (notificationsModel.get(i).source == source) {
                notificationsModel.remove(i)
                break
            }
        }
        if (notificationsModel.count > 20) {
            notificationsModel.remove(notificationsModel.count-1)
        }
        var notification = {"source"  : source,
                "appIcon" : appIcon,
                "image"   : image,
                "appName" : appName,
                "summary" : summary,
                "body"    : body,
                "isPersistent" : isPersistent,
                "expireTimeout": expireTimeout,
                "urgency" : urgency,
                "configurable": configurable,
                "appRealName": appRealName,
                "actions" : actions}

        if (isPersistent) {
            notificationsModel.inserting = true;
            notificationsModel.insert(0, notification);
            notificationsModel.inserting = false;
        }

        if (plasmoid.popupShowing) {
            return
        }

        var popup = notificationPopupComponent.createObject();
        popup.populatePopup(notification);
        notificationPositioner.positionPopup(popup);
    }

    function executeAction(source, id) {
        //try to use the service
        if (source.indexOf("notification") !== -1) {
            var service = notificationsSource.serviceForSource(source)
            var op = service.operationDescription("invokeAction")
            op["actionId"] = id

            service.startOperationCall(op)
        //try to open the id as url
        } else if (source.indexOf("Job") !== -1) {
            Qt.openUrlExternally(id)
        }
    }

    function configureNotification(appRealName) {
        var service = notificationsSource.serviceForSource("notification")
        var op = service.operationDescription("configureNotification")
        op["appRealName"] = appRealName;
        service.startOperationCall(op)
    }

    function closeNotification(source) {
        print("--------------------------------------closing");
        var service = notificationsSource.serviceForSource(source)
        var op = service.operationDescription("userClosed")
        service.startOperationCall(op)
    }

    Component {
        id: notificationPopupComponent
        NotificationPopup {
        }
    }

    ListModel {
        id: notificationsModel
        property bool inserting: false;
    }

    PlasmaCore.DataSource {
        id: idleTimeSource

        property bool idle: data["UserActivity"]["IdleTime"] > 300000

        engine: "powermanagement"
        interval: 30000
        connectedSources: ["UserActivity"]
        //Idle whith more than 5 minutes of user inactivity
    }

    PlasmaCore.DataSource {
        id: notificationsSource

        engine: "notifications"
        interval: 0

        onSourceAdded: {
            print(" +++++++++ connecting " + source);
            connectSource(source);
        }

        onSourceRemoved: {
            notificationPositioner.closePopup(source);

            for (var i = 0; i < notificationsModel.count; ++i) {
                if (notificationsModel.get(i).source == source) {
                    notificationsModel.remove(i)
                    break
                }
            }
        }

        onNewData: {
            var _data = data; // Temp copy to avoid lots of context switching
            var actions = new Array()
            if (data["actions"] && data["actions"].length % 2 == 0) {
                for (var i = 0; i < data["actions"].length; i += 2) {
                    var action = new Object()
                    action["id"] = data["actions"][i]
                    action["text"] = data["actions"][i+1]
                    actions.push(action)
                }
            }
            notificationsRoot.addNotification(
                    sourceName,
                    _data["appIcon"],
                    _data["image"],
                    _data["appName"],
                    _data["summary"],
                    _data["body"],
                    _data["isPersistent"],
                    _data["expireTimeout"],
                    _data["urgency"],
                    _data["appRealName"],
                    _data["configurable"],
                    actions)
        }

    }

    NotificationsHelper {
        id: notificationPositioner
        plasmoidScreen: plasmoid.screen
    }

    Repeater {
        id: notificationsRepeater
        model: notificationsModel
        delegate: NotificationDelegate {
            toolIconSize: notificationsApplet.toolIconSize
        }
    }
}
