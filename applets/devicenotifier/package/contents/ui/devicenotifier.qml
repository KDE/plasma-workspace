/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kquickcontrolsaddons 2.0 // For KCMShell

Item {
    id: devicenotifier

    readonly property bool openAutomounterKcmAuthorized: KCMShell.authorize("device_automounter_kcm.desktop").length > 0

    property string devicesType: {
        if (plasmoid.configuration.allDevices) {
            return "all"
        } else if (plasmoid.configuration.removableDevices) {
            return "removable"
        } else {
            return "nonRemovable"
        }
    }
    property string popupIcon: "device-notifier"

    property bool itemClicked: false
    property int currentIndex: -1
    property var connectedRemovables: []
    property int mountedRemovables: 0

    Plasmoid.switchWidth: PlasmaCore.Units.gridUnit * 10
    Plasmoid.switchHeight: PlasmaCore.Units.gridUnit * 10

    Plasmoid.toolTipMainText: filterModel.count > 0 && filterModel.get(0) ? i18n("Most Recent Device") : i18n("No Devices Available")
    Plasmoid.toolTipSubText: {
        if (filterModel.count > 0) {
            var data = filterModel.get(0)
            if (data && data.Description) {
                return data.Description
            }
        }
        return ""
    }
    Plasmoid.icon: {
        if (filterModel.count > 0) {
            var data = filterModel.get(0)
            if (data && data.Icon) {
                return data.Icon
            }
        }
        return "device-notifier"
    }

    Plasmoid.status: (filterModel.count > 0) ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus

    PlasmaCore.DataSource {
        id: hpSource
        engine: "hotplug"
        connectedSources: sources
        interval: 0

        onSourceAdded: {
            disconnectSource(source);
            connectSource(source);
            sdSource.connectedSources = sources
        }
        onSourceRemoved: {
            disconnectSource(source);
        }
    }

    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
        source: devicenotifier.popupIcon
        width: PlasmaCore.Units.iconSizes.medium;
        height: PlasmaCore.Units.iconSizes.medium;
        active: compactMouse.containsMouse
        MouseArea {
            id: compactMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: plasmoid.expanded = !plasmoid.expanded
        }
    }
    Plasmoid.fullRepresentation: FullRepresentation {}

    PlasmaCore.DataSource {
        id: sdSource
        engine: "soliddevice"
        interval: 0
        property string last
        onSourceAdded: {
            disconnectSource(source);
            connectSource(source);
            last = source;
            processLastDevice(true);
            if (data[source].Removable) {
                devicenotifier.connectedRemovables.push(source);
                devicenotifier.connectedRemovables = devicenotifier.connectedRemovables;
            }
        }

        onSourceRemoved: {
            disconnectSource(source);
            var index = devicenotifier.connectedRemovables.indexOf(source);
            if (index >= 0) {
                devicenotifier.connectedRemovables.splice(index, 1);
                devicenotifier.connectedRemovables = devicenotifier.connectedRemovables;
            }
        }

        onDataChanged: {
            processLastDevice(true);
            var counter = 0;
            for (var i = 0; i < devicenotifier.connectedRemovables.length; i++) {
                if (isMounted(devicenotifier.connectedRemovables[i])) {
                    counter++;
                }
            }
            if (counter !== devicenotifier.mountedRemovables) {
                devicenotifier.mountedRemovables = counter;
            }
        }

        onNewData: {
            last = sourceName;
            processLastDevice(false);
        }

        function isViableDevice(udi) {
            if (devicesType === "all") {
                return true;
            }

            var device = data[udi];
            if (!device) {
                return false;
            }

            return (devicesType === "removable" && device.Removable)
                || (devicesType === "nonRemovable" && !device.Removable);
        }

        function processLastDevice(expand) {
            if (last && isViableDevice(last)) {
                if (expand && hpSource.data[last] && hpSource.data[last].added) {
                    devicenotifier.popupIcon = "preferences-desktop-notification";
                    expandTimer.restart();
                    popupIconTimer.restart();
                }
                last = "";
            }
        }
    }

    PlasmaCore.SortFilterModel {
        id: filterModel
        sourceModel: PlasmaCore.DataModel {
            dataSource: sdSource
        }
        filterRole: "Removable"
        filterRegExp: {
            if (devicesType === "removable") {
                return "true"
            } else if (devicesType === "nonRemovable") {
                return "false"
            } else {
                return ""
            }
        }
        sortRole: "Timestamp"
        sortOrder: Qt.DescendingOrder
    }

    PlasmaCore.DataSource {
        id: statusSource
        engine: "devicenotifications"
        property string last
        property string lastUdi
        property string lastIcon
        property string lastSummary
        onSourceAdded: {
            last = source;
            disconnectSource(source);
            connectSource(source);
        }
        onSourceRemoved: disconnectSource(source)
        onDataChanged: {
            if (last) {
                lastUdi = data[last].udi
                lastIcon = sdSource.data[lastUdi] ? sdSource.data[lastUdi].Icon : "device-notifier"
                lastSummary = sdSource.data[lastUdi] ? sdSource.data[lastUdi].Vendor + " " + sdSource.data[lastUdi].Product : ""

                if (sdSource.isViableDevice(lastUdi)) {
                    plasmoid.expanded = true
                    plasmoid.fullRepresentationItem.spontaneousOpen = true;
                }
            }
        }

        function clearMessage() {
            last = ""
            lastUdi = ""
            lastIcon = ""
            lastSummary = ""
        }
    }

    PlasmaCore.DataSource {
        id: notificationSource
        engine: "notifications"
    }

    property var showRemovableDevicesAction
    property var showNonRemovableDevicesAction
    property var showAllDevicesAction
    property var openAutomaticallyAction

    Component.onCompleted: {
        if (sdSource.connectedSources.count === 0) {
            Plasmoid.status = PlasmaCore.Types.PassiveStatus;
        }

        plasmoid.setAction("unmountAllDevices", i18n("Remove All"), "media-eject");
        plasmoid.action("unmountAllDevices").visible = Qt.binding(() => {
            return devicenotifier.mountedRemovables > 1;
        });
 
        plasmoid.setActionSeparator("sep0");

        plasmoid.setAction("showRemovableDevices", i18n("Removable Devices"), "drive-removable-media");
        devicenotifier.showRemovableDevicesAction = plasmoid.action("showRemovableDevices");
        devicenotifier.showRemovableDevicesAction.checkable = true;
        devicenotifier.showRemovableDevicesAction.checked = Qt.binding(() => {return plasmoid.configuration.removableDevices;});
        plasmoid.setActionGroup("showRemovableDevices", "devicesShown");

        plasmoid.setAction("showNonRemovableDevices", i18n("Non Removable Devices"), "drive-harddisk");
        devicenotifier.showNonRemovableDevicesAction = plasmoid.action("showNonRemovableDevices");
        devicenotifier.showNonRemovableDevicesAction.checkable = true;
        devicenotifier.showNonRemovableDevicesAction.checked = Qt.binding(() => {return plasmoid.configuration.nonRemovableDevices;});
        plasmoid.setActionGroup("showNonRemovableDevices", "devicesShown");

        plasmoid.setAction("showAllDevices", i18n("All Devices"));
        devicenotifier.showAllDevicesAction = plasmoid.action("showAllDevices");
        devicenotifier.showAllDevicesAction.checkable = true;
        devicenotifier.showAllDevicesAction.checked = Qt.binding(() => {return plasmoid.configuration.allDevices;});
        plasmoid.setActionGroup("showAllDevices", "devicesShown");

        plasmoid.setActionSeparator("sep");

        plasmoid.setAction("openAutomatically", i18n("Show popup when new device is plugged in"));
        devicenotifier.openAutomaticallyAction = plasmoid.action("openAutomatically");
        devicenotifier.openAutomaticallyAction.checkable = true;
        devicenotifier.openAutomaticallyAction.checked = Qt.binding(() => {return plasmoid.configuration.popupOnNewDevice;});

        plasmoid.setActionSeparator("sep2");

        if (devicenotifier.openAutomounterKcmAuthorized) {
            plasmoid.removeAction("configure");
            plasmoid.setAction("configure", i18nc("Open auto mounter kcm", "Configure Removable Devices…"), "configure")
        }
    }

    function action_configure() {
        KCMShell.openSystemSettings("kcm_device_automounter")
    }

    function action_showRemovableDevices() {
        plasmoid.configuration.removableDevices = true;
        plasmoid.configuration.nonRemovableDevices = false;
        plasmoid.configuration.allDevices = false;
    }

    function action_showNonRemovableDevices() {
        plasmoid.configuration.removableDevices = false;
        plasmoid.configuration.nonRemovableDevices = true;
        plasmoid.configuration.allDevices = false;
    }

    function action_showAllDevices() {
        plasmoid.configuration.removableDevices = false;
        plasmoid.configuration.nonRemovableDevices = false;
        plasmoid.configuration.allDevices = true;
    }

    function action_openAutomatically() {
        plasmoid.configuration.popupOnNewDevice = !plasmoid.configuration.popupOnNewDevice;
    }

    Plasmoid.onExpandedChanged: {
        popupEventSlot(plasmoid.expanded);
    }

    function popupEventSlot(popped) {
        if (!popped) {
            // reset the property that lets us remember if an item was clicked
            // (versus only hovered) for autohide purposes
            devicenotifier.itemClicked = true;
            devicenotifier.currentIndex = -1;
        }
    }

    function isMounted(udi) {
        if (!sdSource.data[udi]) {
            return false;
        }

        var types = sdSource.data[udi]["Device Types"];
        if (types.indexOf("Storage Access") >= 0) {
            return sdSource.data[udi]["Accessible"];
        }

        return (types.indexOf("Storage Volume") >= 0 && types.indexOf("OpticalDisc") >= 0)
    }

    Timer {
        id: popupIconTimer
        interval: 3000
        onTriggered: devicenotifier.popupIcon  = "device-notifier";
    }

    Timer {
        id: expandTimer
        interval: 250
        onTriggered: {
            // We don't show a UI for it, but there is a hidden option to not
            // show the popup on new device attachment if the user has added
            // the text "popupOnNewDevice=false" to their
            // plasma-org.kde.plasma.desktop-appletsrc file.
            if (plasmoid.configuration.popupOnNewDevice) { // Bug 351592
                plasmoid.expanded = true;
                plasmoid.fullRepresentationItem.spontaneousOpen = true;
            }
        }
    }

}
