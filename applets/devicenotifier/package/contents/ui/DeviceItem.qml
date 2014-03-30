/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2012 Jacopo De Simoi <wilderkde@gmail.com>
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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0

Item {
    id: deviceItem
    property string udi
    property string icon
    property alias deviceName: deviceLabel.text
    property string emblemIcon
    property int state
    property alias leftActionIcon: leftAction.source
    property bool mounted
    property bool expanded: (ListView.view.currentExpanded == index)
    property alias percentUsage: freeSpaceBar.value
    property string freeSpaceText
    signal leftActionTriggered

    height: container.childrenRect.height + padding.margins.top + padding.margins.bottom
    width: parent.width

    PlasmaCore.FrameSvgItem {
        id: padding
        imagePath: "widgets/viewitem"
        prefix: "hover"
        opacity: 0
        anchors.fill: parent
    }
    MouseArea {
        id: container
        anchors {
            fill: parent
            topMargin: padding.margins.top
            leftMargin: padding.margins.left
            rightMargin: padding.margins.right
            bottomMargin: padding.margins.bottom
        }

        hoverEnabled: true
        onEntered: {
            deviceItem.ListView.view.currentIndex = index;

            //this is done to hide the highlight if the mouse moves out of the list view
            //and we are not mouseoverring anything
            if (deviceItem.ListView.view.highlightItem) {
                deviceItem.ListView.view.highlightItem.opacity = 1
            }
        }
        onExited: {
            if (deviceItem.ListView.view.highlightItem) {
                deviceItem.ListView.view.highlightItem.opacity = 0
            }
        }
        onClicked: {
            var actions = hpSource.data[udi]["actions"];
            if (actions.length == 1) {
                var service = hpSource.serviceForSource(udi);
                var operation = service.operationDescription("invokeAction");
                operation.predicate = actions[0]["predicate"];
                service.startOperationCall(operation);
            } else {
                deviceItem.ListView.view.currentExpanded = expanded ? -1 : index;
            }
        }

        // FIXME: Device item loses focus on mounting/unmounting it,
        // or specifically, when some UI element changes.
        PlasmaCore.IconItem {
            id: deviceIcon
            width: units.iconSizes.medium
            height: width
            z: 900
            source: deviceItem.icon
            enabled: deviceItem.state == 0
            anchors {
                left: parent.left
                top: parent.top
            }

            PlasmaCore.IconItem {
                id: emblem
                width: units.iconSizes.medium * 0.5
                height: width
                source: deviceItem.state == 0 ? emblemIcon : undefined;
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                }
            }
        }


        Column {
            id: labelsColumn
            spacing: padding.margins.top/2
            z: 900
            anchors {
                top: parent.top
                left: deviceIcon.right
                right: leftActionArea.left
                leftMargin: padding.margins.left
                rightMargin: padding.margins.right
            }

            PlasmaComponents.Label {
                id: deviceLabel
                height: paintedHeight
                wrapMode: Text.WordWrap
                anchors {
                    left: parent.left
                    right: parent.right
                }
                enabled: deviceItem.state == 0
            }

            PlasmaCore.ToolTipArea {
                id: freeSpaceToolip
                height:freeSpaceBar.height

                subText: i18nc("@info:status Free disk space", "%1 free", deviceItem.freeSpaceText)
                anchors {
                    left: parent.left
                    right: parent.right
                }



                opacity: (deviceItem.state == 0 && mounted) ? 1 : 0
                PlasmaComponents.ProgressBar {
                    id: freeSpaceBar
                    height: deviceStatus.height
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    opacity: mounted ? deviceStatus.opacity : 0
                    minimumValue: 0
                    maximumValue: 100
                    orientation: Qt.Horizontal
                }
            }

            Item {
                width:deviceStatus.width
                height:deviceStatus.height
                PlasmaComponents.Label {
                    id: deviceStatus

                    height: paintedHeight
                    // FIXME: state changes do not reach the plasmoid if the
                    // device was already attached when the plasmoid was
                    // initialized
                    text: deviceItem.state == 0 ? container.idleStatus() : (deviceItem.state==1 ? i18nc("Accessing is a less technical word for Mounting; translation should be short and mean \'Currently mounting this device\'", "Accessing...") : i18nc("Removing is a less technical word for Unmounting; translation shoud be short and mean \'Currently unmounting this device\'", "Removing..."))
                    font.pointSize: theme.smallestFont.pointSize
                    color: "#99"+(theme.textColor.toString().substr(1))
                    opacity: deviceItem.state != 0 || container.containsMouse || expanded ? 1 : 0;

                    Behavior on opacity { NumberAnimation { duration: units.shortDuration * 3 } }
                }
            }
        }

        function idleStatus() {
            var actions = hpSource.data[udi]["actions"];
            if (actions.length > 1) {
                return i18np("1 action for this device", "%1 actions for this device", actions.length);
            } else {
                return actions[0]["text"];
            }
        }


        MouseEventListener {
            id: leftActionArea
            width: units.iconSizes.medium*0.8
            height: width
            hoverEnabled: true
            anchors {
                right: parent.right
                verticalCenter: deviceIcon.verticalCenter
            }

            onClicked: {
                if (leftAction.visible) {
                    leftActionTriggered()
                }
            }

            PlasmaCore.IconItem {
                id: leftAction
                anchors.fill: parent
                active: leftActionArea.containsMouse
                visible: !busySpinner.visible

                PlasmaCore.ToolTipArea {
                    anchors.fill: leftAction
                    subText: {
                        if (!model["Accessible"]) {
                            return i18n("Click to mount this device.")
                        } else if (model["Device Types"].indexOf("OpticalDisc") != -1) {
                            return i18n("Click to eject this disc.")
                        } else if (model["Removable"]) {
                            return i18n("Click to safely remove this device.")
                        } else {
                            return i18n("Click to access this device from other applications.")
                        }
                    }
                }
            }

            PlasmaComponents.BusyIndicator {
                id: busySpinner
                anchors.fill: parent
                running: visible
                visible: deviceItem.state != 0
            }
        }

        PlasmaCore.ToolTipArea {
            anchors.fill: deviceIcon
            subText: {
                if (model["Accessible"] || deviceItem.state != 0) {
                    if (model["Removable"]) {
                        return i18n("It is currently <b>not safe</b> to remove this device: applications may be accessing it. Click the eject button to safely remove this device.")
                    } else {
                        return i18n("This device is currently accessible.")
                    }
                } else {
                    if (model["Removable"]) {
                        if (model["In Use"]) {
                            return i18n("It is currently <b>not safe</b> to remove this device: applications may be accessing other volumes on this device. Click the eject button on these other volumes to safely remove this device.");
                        } else {
                            return i18n("It is currently safe to remove this device.")
                        }
                    } else {
                        return i18n("This device is not currently accessible.")
                    }
                }
            }
        }

        ListView {
            id: actionsList
            anchors {
                top: labelsColumn.bottom
                left: deviceIcon.right
                right: leftActionArea.left
            }
            interactive: false
            model: hpSource.data[udi]["actions"]
            property int actionVerticalMargins: 5
            property int actionIconHeight: units.iconSizes.medium*0.8
            height: expanded ? ((actionIconHeight+(2*actionVerticalMargins))*model.length)+anchors.topMargin : 0
            opacity: expanded ? 1 : 0
            delegate: actionItem
            highlight: PlasmaComponents.Highlight{}
            Behavior on opacity { NumberAnimation { duration: units.shortDuration * 3 } }

            Component.onCompleted: currentIndex = -1
        }

        Component {
            id: actionItem

            ActionItem {
                icon: modelData["icon"]
                label: modelData["text"]
                predicate: modelData["predicate"]
            }
        }
    }
}
