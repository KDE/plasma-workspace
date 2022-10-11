/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kquickcontrolsaddons 2.0 // For KCMShell

Item {
    id: devicenotifier

    readonly property bool openAutomounterKcmAuthorized: KCMShell.authorize("device_automounter_kcm.desktop").length > 0

    property string devicesType: {
        if (Plasmoid.configuration.allDevices) {
            return "all"
        } else if (Plasmoid.configuration.removableDevices) {
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

    // QTBUG-50380: As soon as the item gets removed from the model, all of ListView's
    // properties (count, contentHeight) pretend the delegate doesn't exist anymore
    // causing our "No devices" heading to overlap with the remaining device
    property bool isMessageHighlightAnimatorRunning: false

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

    Plasmoid.status: (filterModel.count > 0 || isMessageHighlightAnimatorRunning) ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus

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
            activeFocusOnTab: true
            hoverEnabled: true
            Accessible.name: Plasmoid.title
            Accessible.description: `${Plasmoid.toolTipMainText}: ${Plasmoid.toolTipSubText}`
            Accessible.role: Accessible.Button
            onClicked: Plasmoid.expanded = !Plasmoid.expanded
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
        property string lastDescription
        property string lastMessage
        property string lastIcon
        onSourceAdded: {
            last = source;
            disconnectSource(source);
            connectSource(source);
        }
        onSourceRemoved: disconnectSource(source)
        onDataChanged: {
            if (last) {
                lastUdi = data[last].udi
                lastDescription = sdSource.data[lastUdi] ? sdSource.data[lastUdi].Description : ""
                lastMessage = data[last].error
                lastIcon = sdSource.data[lastUdi] ? sdSource.data[lastUdi].Icon : "device-notifier"

                if (sdSource.isViableDevice(lastUdi)) {
                    Plasmoid.expanded = true
                    Plasmoid.fullRepresentationItem.spontaneousOpen = true;
                }
            }
        }

        function clearMessage() {
            last = ""
            lastUdi = ""
            lastDescription = ""
            lastMessage = ""
            lastIcon = ""
        }
    }

    property var showRemovableDevicesAction
    property var showNonRemovableDevicesAction
    property var showAllDevicesAction
    property var openAutomaticallyAction

    Component.onCompleted: {
        if (sdSource.connectedSources.count === 0) {
            Plasmoid.status = PlasmaCore.Types.PassiveStatus;
        }

        Plasmoid.setAction("unmountAllDevices", i18n("Remove All"), "media-eject");
        Plasmoid.action("unmountAllDevices").visible = Qt.binding(() => {
            return devicenotifier.mountedRemovables > 0;
        });
 
        Plasmoid.setActionSeparator("sep0");

        Plasmoid.setAction("showRemovableDevices", i18n("Removable Devices"), "drive-removable-media");
        devicenotifier.showRemovableDevicesAction = Plasmoid.action("showRemovableDevices");
        devicenotifier.showRemovableDevicesAction.checkable = true;
        devicenotifier.showRemovableDevicesAction.checked = Qt.binding(() => {return Plasmoid.configuration.removableDevices;});
        Plasmoid.setActionGroup("showRemovableDevices", "devicesShown");

        Plasmoid.setAction("showNonRemovableDevices", i18n("Non Removable Devices"), "drive-harddisk");
        devicenotifier.showNonRemovableDevicesAction = Plasmoid.action("showNonRemovableDevices");
        devicenotifier.showNonRemovableDevicesAction.checkable = true;
        devicenotifier.showNonRemovableDevicesAction.checked = Qt.binding(() => {return Plasmoid.configuration.nonRemovableDevices;});
        Plasmoid.setActionGroup("showNonRemovableDevices", "devicesShown");

        Plasmoid.setAction("showAllDevices", i18n("All Devices"));
        devicenotifier.showAllDevicesAction = Plasmoid.action("showAllDevices");
        devicenotifier.showAllDevicesAction.checkable = true;
        devicenotifier.showAllDevicesAction.checked = Qt.binding(() => {return Plasmoid.configuration.allDevices;});
        Plasmoid.setActionGroup("showAllDevices", "devicesShown");

        Plasmoid.setActionSeparator("sep");

        Plasmoid.setAction("openAutomatically", i18n("Show popup when new device is plugged in"));
        devicenotifier.openAutomaticallyAction = Plasmoid.action("openAutomatically");
        devicenotifier.openAutomaticallyAction.checkable = true;
        devicenotifier.openAutomaticallyAction.checked = Qt.binding(() => {return Plasmoid.configuration.popupOnNewDevice;});

        Plasmoid.setActionSeparator("sep2");

        if (devicenotifier.openAutomounterKcmAuthorized) {
            Plasmoid.removeAction("configure");
            Plasmoid.setAction("configure", i18nc("Open auto mounter kcm", "Configure Removable Devices…"), "configure")
        }
    }

    function action_configure() {
        KCMShell.openSystemSettings("kcm_device_automounter")
    }

    function action_showRemovableDevices() {
        Plasmoid.configuration.removableDevices = true;
        Plasmoid.configuration.nonRemovableDevices = false;
        Plasmoid.configuration.allDevices = false;
    }

    function action_showNonRemovableDevices() {
        Plasmoid.configuration.removableDevices = false;
        Plasmoid.configuration.nonRemovableDevices = true;
        Plasmoid.configuration.allDevices = false;
    }

    function action_showAllDevices() {
        Plasmoid.configuration.removableDevices = false;
        Plasmoid.configuration.nonRemovableDevices = false;
        Plasmoid.configuration.allDevices = true;
    }

    function action_openAutomatically() {
        Plasmoid.configuration.popupOnNewDevice = !Plasmoid.configuration.popupOnNewDevice;
    }

    Plasmoid.onExpandedChanged: {
        popupEventSlot(Plasmoid.expanded);
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
            if (Plasmoid.configuration.popupOnNewDevice) { // Bug 351592
                Plasmoid.expanded = true;
                Plasmoid.fullRepresentationItem.spontaneousOpen = true;
            }
        }
    }

}
