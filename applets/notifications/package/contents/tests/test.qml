import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0

Item {
    id: root
    property QtObject plasmoidItem: null
    onPlasmoidItemChanged: {
        if (plasmoidItem && !root.plasmoidItem.rootItem) {
            discardTimer.running = true
            return
        }

        showNotification("testing test")
        console.log("sent notification", root.plasmoidItem, root.plasmoidItem.rootItem)
    }
    Timer {
        id: discardTimer
        interval: 0
        running: false
        onTriggered: {
            notificationShown = notificationClosed = true
            root.done();
        }
    }

    PlasmaCore.DataSource {
        id: notificationSource
        engine: "notifications"
        connectedSources: "org.freedesktop.Notifications"
    }

    function showNotification(summary, icon, appname, body, timeout) {
        if(!icon) icon = "debug-run"
        if(!appname) appname="test"
        if(!body) body=""
        if(!timeout) timeout=2000

        var service = notificationSource.serviceForSource("notification");
        var operation = service.operationDescription("createNotification");
        operation["appName"] = appname;
        operation["appIcon"] = icon;
        operation["summary"] = summary;
        operation["body"] = body;
        operation["timeout"] = timeout;

        service.startOperationCall(operation);
    }

    Connections {
        target: root.plasmoidItem.rootItem ? root.plasmoidItem.rootItem.notifications : null
        onPopupShown: {
            root.notificationShown = true
            popupConnections.target = popup
        }
    }

    Connections {
        id: popupConnections
        onVisibleChanged: {
            if (target.visible) {
                popupConnections.target.mainItem.close()
            } else {
                root.notificationClosed = true
                root.done()
            }
        }
    }

    property bool notificationShown: false
    property bool notificationClosed: false
    signal done()
    readonly property bool failed: !notificationShown || !notificationClosed
}
