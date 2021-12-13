/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Controls 2.12 as QQC2
import QtQml.Models 2.14

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kquickcontrolsaddons 2.0

PlasmaExtras.ExpandableListItem {
    id: deviceItem

    property string udi
    readonly property int state: sdSource.data[udi] ? sdSource.data[udi].State : 0
    readonly property int operationResult: (model["Operation result"])

    readonly property bool isMounted: devicenotifier.isMounted(udi)
    readonly property bool hasMessage: statusSource.lastUdi == udi && statusSource.data[statusSource.last] ? true : false
    readonly property var message: hasMessage ? statusSource.data[statusSource.last] || ({}) : ({})

    readonly property double freeSpace: sdSource.data[udi] && sdSource.data[udi]["Free Space"] ? sdSource.data[udi]["Free Space"] : -1.0
    readonly property double totalSpace: sdSource.data[udi] && sdSource.data[udi]["Size"] ? sdSource.data[udi]["Size"] : -1.0
    property bool freeSpaceKnown: freeSpace > 0 && totalSpace > 0

    readonly property bool isRootVolume: sdSource.data[udi] && sdSource.data[udi]["File Path"] ? sdSource.data[udi]["File Path"] == "/" : false
    readonly property bool isRemovable: sdSource.data[udi] && sdSource.data[udi]["Removable"] ? sdSource.data[udi]["Removable"] : false

    onOperationResultChanged: {
        if (!popupIconTimer.running) {
            if (operationResult == 1) {
                devicenotifier.popupIcon = "dialog-ok"
                popupIconTimer.restart()
            } else if (operationResult == 2) {
                devicenotifier.popupIcon = "dialog-error"
                popupIconTimer.restart()
            }
        }
    }

    onHasMessageChanged: {
        if (deviceItem.hasMessage) {
            messageHighlight.highlight(this)
        }
    }

    Connections {
        target: unmountAll
        function onClicked() {
            if (model["Removable"] && isMounted) {
                actionTriggered();
            }
        }
    }

    Connections {
         target: plasmoid.action("unmountAllDevices")
         function onTriggered() {
             if (model["Removable"] && isMounted) {
                 actionTriggered();
             }
         }
     }

    /**
     * BUG 427945: It's very dangerous and racey to access and set properties in ListView.onRemove,
     * so just show a notification.
     */
    ListView.onRemove: {
        if (!(statusSource.lastUdi === udi && statusSource.data[statusSource.last])) {
            return;
        }

        let notifications = notificationSource.serviceForSource("notification");
        let operation = notifications.operationDescription("createNotification");
        operation.appName = i18nc("@title notification title", "Disks & Devices");
        operation.appIcon = statusSource.lastIcon;
        operation.summary = statusSource.lastSummary;
        operation.body = statusSource.data[statusSource.last].error;
        notifications.startOperationCall(operation);
        statusSource.clearMessage();

        // Otherwise there are briefly multiple highlight effects
        devicenotifier.currentIndex = -1
    }

    Timer {
        id: updateStorageSpaceTimer
        interval: 5000
        repeat: true
        running: isMounted && plasmoid.expanded
        triggeredOnStart: true     // Update the storage space as soon as we open the plasmoid
        onTriggered: {
            var service = sdSource.serviceForSource(udi);
            var operation = service.operationDescription("updateFreespace");
            service.startOperationCall(operation);
        }
    }

    Component {
        id: deviceActionComponent
        QQC2.Action { }
    }

    function actionTriggered() {
        var service
        var operationName
        var operation
        var wasMounted = isMounted;
        if (!isRemovable || !isMounted) {
            service = hpSource.serviceForSource(udi);
            operation = service.operationDescription('invokeAction');
            operation.predicate = "test-predicate-openinwindow.desktop";
        } else {
            service = sdSource.serviceForSource(udi);
            operation = service.operationDescription("unmount");
        }
        service.startOperationCall(operation);
        if (wasMounted) {
            deviceItem.collapse();
        }
    }


    // When there's no better icon available, show a placeholder icon instead
    // of nothing
    icon: sdSource.data[udi] ? sdSource.data[udi].Icon : "device-notifier"

    iconEmblem: {
        if (deviceItem.hasMessage) {
            if (deviceItem.message.solidError === 0) {
                return "emblem-information"
            } else {
                return "emblem-error"
            }
        } else if (deviceItem.state == 0 && Emblems && Emblems[0]) {
            return Emblems[0]
        } else {
            return ""
        }
    }

    title: sdSource.data[udi] ? sdSource.data[udi].Description : ""

    subtitle: {
        if (deviceItem.hasMessage) {
            return deviceItem.message.error
        }
        if (deviceItem.state == 0) {
            if (!hpSource.data[udi]) {
                return ""
            }
            if (freeSpaceKnown) {
                var freeSpaceText = sdSource.data[udi]["Free Space Text"]
                var totalSpaceText = sdSource.data[udi]["Size Text"]
                return i18nc("@info:status Free disk space", "%1 free of %2", freeSpaceText, totalSpaceText)
            }
            return ""
        } else if (deviceItem.state == 1) {
            return i18nc("Accessing is a less technical word for Mounting; translation should be short and mean \'Currently mounting this device\'", "Accessing…")
        } else {
            return i18nc("Removing is a less technical word for Unmounting; translation should be short and mean \'Currently unmounting this device\'", "Removing…")
        }
    }

    subtitleCanWrap: true

    // Color the subtitle red for disks with less than 5% free space
    subtitleColor: {
        if (freeSpaceKnown) {
            if (freeSpace / totalSpace <= 0.05) {
                return PlasmaCore.Theme.negativeTextColor
            }
        }
        return PlasmaCore.Theme.textColor
    }

    defaultActionButtonAction: QQC2.Action {
        icon.name: {
            if (isRemovable) {
                return isMounted ? "media-eject" : "document-open-folder"
            } else {
                return "document-open-folder"
            }
        }
        text: {
            // It's possible for the root volume to be on a removable disk
            if (!isRemovable || isRootVolume) {
                return i18n("Open in File Manager")
            } else {
                var types = model["Device Types"];
                if (!isMounted) {
                    return i18n("Mount and Open")
                } else if (types && types.indexOf("OpticalDisc") !== -1) {
                    return i18n("Eject")
                } else {
                    return i18n("Safely remove")
                }
            }
        }
        onTriggered: actionTriggered()
    }

    isBusy: deviceItem.state != 0

    // We need a JS array full of QQC2 actions; this Instantiator creates them
    // from the actions list of the data source
    Instantiator {
        model: hpSource.data[udi] ? hpSource.data[udi].actions : []
        delegate: QQC2.Action {
            text: modelData.text
            icon.name: modelData.icon
            // We only want to show the "Show in file manager" action for
            // mounted removable disks (for everything else, this action is
            // already the primary one shown on the list item)
            enabled: {
                if (modelData.predicate != "test-predicate-openinwindow.desktop") {
                    return true;
                }
                return deviceItem.isRemovable && deviceItem.isMounted;
            }
            onTriggered: {
                var service = hpSource.serviceForSource(udi);
                var operation = service.operationDescription('invokeAction');
                operation.predicate = modelData.predicate;
                service.startOperationCall(operation);
                devicenotifier.currentIndex = -1;
            }
        }
        onObjectAdded: contextualActionsModel.push(object)
    }

    // "Mount" action that does not open it in the file manager
    QQC2.Action {
        id: mountWithoutOpeningAction

        text: i18n("Mount")
        icon.name: "media-mount"

        // Only show for unmounted removable devices
        enabled: deviceItem.isRemovable && !deviceItem.isMounted

        onTriggered: {
            var service = sdSource.serviceForSource(udi);
            var operation = service.operationDescription("mount");
            service.startOperationCall(operation);
        }
    }

    Component.onCompleted: {
        contextualActionsModel.push(mountWithoutOpeningAction);
    }
}
