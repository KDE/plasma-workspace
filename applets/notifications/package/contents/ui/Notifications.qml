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
import QtQuick.Layouts 1.2
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.plasma.private.notifications 1.0


Column {
    id: notificationsRoot
    anchors {
        left: parent.left
        right: parent.right
    }

    property alias count: notificationsRepeater.count
    readonly property int historyCount: historyList.count
    
    property bool showHistory: plasmoid.configuration.showHistory
    
    signal popupShown(var popup)
    
    onShowHistoryChanged: {
        if(!showHistory)
            clearHistory()
    }

    Component.onCompleted: {
        // Create the popup components and pass them to the C++ plugin
        for (var i = 0; i < 3; i++) {
            var popup = notificationPopupComponent.createObject();
            notificationPositioner.addNotificationPopup(popup);
        }
    }

    function addNotification(notification) {
        // Do not show duplicated notifications
        for (var i = 0; i < notificationsModel.count; ++i) {
            if (notificationsModel.get(i).source == notification.source &&
                notificationsModel.get(i).appName == notification.appName &&
                notificationsModel.get(i).summary == notification.summary &&
                notificationsModel.get(i).body == notification.body) {
                return
            }
        }

        for (var i = 0; i < notificationsModel.count; ++i) {
            if (notificationsModel.get(i).source == notification.source ||
                (notificationsModel.get(i).appName == notification.appName &&
                notificationsModel.get(i).summary == notification.summary &&
                notificationsModel.get(i).body == notification.body)) {

                notificationsModel.remove(i)
                break
            }
        }
        if (notificationsModel.count > 20) {
            notificationsModel.remove(notificationsModel.count-1)
        }

        if (notification.isPersistent) {
            notification.created = new Date();

            notificationsModel.inserting = true;
            notificationsModel.insert(0, notification);
            notificationsModel.inserting = false;
        }
        else if (showHistory) {
            
            notificationsHistoryModel.inserting = true;

            //Disable actions in this copy as they will stop working once the original notification is closed.
            //Only the jobUrl (which is a URL to open) can continue working as we'll handle this internally.
            var actions = notification.actions.filter(function (item) {
                return item.id.indexOf("jobUrl#") === 0;
            });
            
            //create a copy of the notification. 
            //Disable actions in this copy as they will stop working once the original notification is closed.
            notificationsHistoryModel.insert(0, {
                "compact" : notification.compact,
                "icon" : notification.icon,
                "image" : notification.image,
                "summary" : notification.summary,
                "body" : notification.body,
                "configurable" : false,
                "created" : new Date(),
                "urls" : notification.urls,
                "maximumTextHeight" : notification.maximumTextHeight,
                "actions" : actions,
                "hasDefaultAction" : false,
                "hasConfigureAction" : false,
            });
            notificationsHistoryModel.inserting = false;
        }

        notificationPositioner.displayNotification(notification);
    }

    function executeAction(source, id) {
        //try to use the service
        if (id.indexOf("jobUrl#") === -1) {
            var service = notificationsSource.serviceForSource(source)
            var op = service.operationDescription("invokeAction")
            op["actionId"] = id

            service.startOperationCall(op)
        //try to open the id as url
        } else if (id.indexOf("jobUrl#") !== -1) {
            Qt.openUrlExternally(id.slice(7));
        }

        notificationPositioner.closePopup(source);
    }

    function configureNotification(appRealName, eventId) {
        var service = notificationsSource.serviceForSource("notification")
        var op = service.operationDescription("configureNotification")
        op.appRealName = appRealName
        op.eventId = eventId
        service.startOperationCall(op)
    }
    function createNotification(data) {
        var service = notificationsSource.serviceForSource("notification");
        var op = service.operationDescription("createNotification");
        // add everything from "data" to "op"
        for (var attrname in data) { op[attrname] = data[attrname]; }
        service.startOperationCall(op);
    }

    function closeNotification(source) {
        var service = notificationsSource.serviceForSource(source)
        var op = service.operationDescription("userClosed")
        service.startOperationCall(op)
    }

    function expireNotification(source) {
        var service = notificationsSource.serviceForSource(source)
        var op = service.operationDescription("expireNotification")
        service.startOperationCall(op)
    }

    function clearNotifications() {
        for (var i = 0, length = notificationsSource.sources.length; i < length; ++i) {
            var source = notificationsSource.sources[i];
            closeNotification(source)
            notificationPositioner.closePopup(source);
        }

        notificationsModel.clear()
        clearHistory()
    }
    
    function clearHistory() {
        notificationsHistoryModel.clear()
    }

    Component {
        id: notificationPopupComponent
        NotificationPopup { }
    }

    ListModel {
        id: notificationsModel
        property bool inserting: false
    }
    
    ListModel {
        id: notificationsHistoryModel
        property bool inserting: false
    }

    PlasmaCore.DataSource {
        id: idleTimeSource

        property bool idle: data["UserActivity"]["IdleTime"] > 300000

        engine: "powermanagement"
        interval: 30000
        connectedSources: ["UserActivity"]
        //Idle with more than 5 minutes of user inactivity
    }

    PlasmaCore.DataSource {
        id: notificationsSource

        engine: "notifications"
        interval: 0

        onSourceAdded: {
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
            var actions = []
            _data["hasDefaultAction"] = false
            _data["hasConfigureAction"] = false;
            if (data["actions"] && data["actions"].length % 2 == 0) {
                for (var i = 0; i < data["actions"].length; i += 2) {
                    var action = data["actions"][i]
                    if (action == "default") { // The default action is not shown, but we want to know it's there
                        _data["hasDefaultAction"] = true
                    } else if (action == "settings") { // configure icon in the notification for custom notification settings
                        _data["hasConfigureAction"] = true;
                        _data["configurable"] = true;
                    } else {
                        actions.push({
                            id: data["actions"][i],
                            text: data["actions"][i+1]
                        })
                    }
                }
            }
            _data["source"] = sourceName
            _data["actions"] = actions
            notificationsRoot.addNotification(_data)
        }

    }

    Connections {
        target: plasmoid.nativeInterface
        onAvailableScreenRectChanged: {
            notificationPositioner.setPlasmoidScreenGeometry(availableScreenRect);
        }
    }

    NotificationsHelper {
        id: notificationPositioner
        popupLocation: plasmoid.nativeInterface.screenPosition

        Component.onCompleted: {
            notificationPositioner.setPlasmoidScreenGeometry(plasmoid.nativeInterface.availableScreenRect);
        }
        onPopupShown: notificationsRoot.popupShown(popup)
    }

    Repeater {
        id: notificationsRepeater
        model: notificationsModel
        delegate: NotificationDelegate { listModel: notificationsModel }
    }
    
    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing
        visible: historyCount > 0
        width: parent.width

        PlasmaExtras.Heading {
            Layout.fillWidth: true
            level: 3
            opacity: 0.6
            text: i18n("History")
        }

   	PlasmaComponents.ToolButton {
            Layout.rightMargin: spacerSvgFrame.margins.right
            iconSource: "edit-clear-history"
            tooltip: i18n("Clear History")
            onClicked: clearHistory()
        }
    }

    // This hack is unfortunately needed to have the buttons align, 
    // the ones in the list contain have a margin due to a frame for being a list item.
    PlasmaCore.FrameSvgItem {
        id : spacerSvgFrame
        imagePath: "widgets/listitem"
        prefix: "normal"
        visible: false
    }

    // History stuff
    // The history is shown outside in a ListView
    Binding {
        target: historyList
        property: "model"
        value: notificationsHistoryModel
        when: showHistory
    }

    Binding {
        target: historyList
        property: "delegate"
        value: notificationsHistoryDelegate
        when: showHistory
    }

    Component {
        id: notificationsHistoryDelegate
        NotificationDelegate {
            listModel: notificationsHistoryModel
        }
    }
}
