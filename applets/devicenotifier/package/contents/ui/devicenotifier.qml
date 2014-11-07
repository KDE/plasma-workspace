/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2012 Jacopo De Simoi <wilderkde@gmail.com>
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: devicenotifier
    property string devicesType: "removable"
    property string expandedDevice
    property string popupIcon: "device-notifier"

    property bool itemClicked: false
    property int currentExpanded: -1
    property int currentIndex: -1

    Plasmoid.switchWidth: units.gridUnit * 10
    Plasmoid.switchHeight: units.gridUnit * 15
    Plasmoid.toolTipMainText: i18n("No devices available")
    Plasmoid.status : (filterModel.count >  0) ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus


    PlasmaCore.DataSource {
        id: hpSource
        engine: "hotplug"
        connectedSources: sources
        interval: 0

        onSourceAdded: {
            disconnectSource(source);
            connectSource(source);
        }
        onSourceRemoved: {
            disconnectSource(source);
        }
    }

    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
        source: devicenotifier.popupIcon
        width: 36;
        height: 36;
        MouseArea {
            anchors.fill: parent
            onClicked: plasmoid.expanded = !plasmoid.expanded
        }
    }
    Plasmoid.fullRepresentation: FullRepresentation {}

    PlasmaCore.DataSource {
        id: sdSource
        engine: "soliddevice"
        connectedSources: hpSource.sources
        interval: 0
        property string last
        onSourceAdded: {
            disconnectSource(source);
            connectSource(source);
            last = source;
            processLastDevice(true);
        }

        onSourceRemoved: {
            if (expandedDevice == source) {
                devicenotifier.currentExpanded = -1;
                expandedDevice = "";
            }
            disconnectSource(source);
        }

        onDataChanged: {
            processLastDevice(true);
        }

        onNewData: {
            last = sourceName;
            processLastDevice(false);
        }

        function processLastDevice(expand) {
            if (last != "") {
                if (devicesType == "all" ||
                    (devicesType == "removable" && data[last] && data[last]["Removable"] == true) ||
                    (devicesType == "nonRemovable" && data[last] && data[last]["Removable"] == false)) {
                    if (expand && hpSource.data[last]["added"]) {
                        expandDevice(last)
                    }
                    last = "";
                }
            }
        }
    }

    PlasmaCore.DataSource {
        id: statusSource
        engine: "devicenotifications"
        property string last
        onSourceAdded: {
            last = source;
            disconnectSource(source);
            connectSource(source);
        }
        onSourceRemoved: {
            disconnectSource(source);
        }
        onDataChanged: {
            if (last != "") {
                statusBar.setData(data[last]["error"], data[last]["errorDetails"], data[last]["udi"]);
                plasmoid.status = PlasmaCore.Types.NeedsAttentionStatus;
                plasmoid.expanded = true;
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
            var all = devicenotifier.Plasmoid.configuration.allDevices;
            var removable = devicenotifier.Plasmoid.configuration.removableDevices;

            if (all == true) {
                devicesType = "all";
                return "";
            } else if (removable == true) {
                devicesType = "removable";
                return "true";
            } else {
                devicesType = "nonRemovable";
                return "false";
            }
        }
        sortRole: "Timestamp"
        sortOrder: Qt.DescendingOrder
        onCountChanged: {
            var data = filterModel.get(0);
            if (data && (data["Icon"] != undefined)) {
                plasmoid.icon = data["Icon"];
                plasmoid.toolTipMainText = i18n("Most recent device");
                plasmoid.toolTipSubText = data["Description"];
            } else {
                plasmoid.icon = "device-notifier";
                plasmoid.toolTipMainText = i18n("No devices available");
                plasmoid.toolTipSubText = "";
            }
        }
    }
    Component.onCompleted: {
        if (sdSource.connectedSources.count == 0) {
            Plasmoid.status = PlasmaCore.Types.PassiveStatus;
        }
    }

    Plasmoid.onExpandedChanged: {
        popupEventSlot(plasmoid.expanded);
    }

    function popupEventSlot(popped) {
        if (!popped) {
            // reset the property that lets us remember if an item was clicked
            // (versus only hovered) for autohide purposes
            devicenotifier.itemClicked = true;
            expandedDevice = "";
            devicenotifier.currentExpanded = -1;
            devicenotifier.currentIndex = -1;
        }
    }

    function expandDevice(udi)
    {
        if (hpSource.data[udi]["actions"].length > 1) {
            expandedDevice = udi
        }

        // reset the property that lets us remember if an item was clicked
        // (versus only hovered) for autohide purposes
        devicenotifier.itemClicked = false;

        devicenotifier.popupIcon = "preferences-desktop-notification";
        //plasmoid.expanded = true;
        expandTimer.restart();
        popupIconTimer.restart()
    }

    function isMounted (udi) {
        var types = sdSource.data[udi]["Device Types"];
        if (types.indexOf("Storage Access")>=0) {
            if (sdSource.data[udi]["Accessible"]) {
                return true;
            }
            else {
                return false;
            }
        }
        else if (types.indexOf("Storage Volume")>=0 && types.indexOf("OpticalDisc")>=0) {
            return true;
        }
        else {
            return false;
        }
    }

    Timer {
        id: popupIconTimer
        interval: 2500
        onTriggered: devicenotifier.popupIcon  = "device-notifier";
    }

    Timer {
        id: passiveTimer
        interval: 2500
        onTriggered: plasmoid.status = PlasmaCore.Types.PassiveStatus
    }

    Timer {
        id: expandTimer
        interval: 250
        onTriggered: {
            plasmoid.expanded = !plasmoid.expanded;
        }
    }

}
