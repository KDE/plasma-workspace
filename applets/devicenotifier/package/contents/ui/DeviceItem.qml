/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2012 Jacopo De Simoi <wilderkde@gmail.com>
 *   Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>
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

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0

MouseArea {
    id: deviceItem

    property string udi
    property string icon
    property alias deviceName: deviceLabel.text
    property string emblemIcon
    property int state

    property bool mounted
    property bool expanded: devicenotifier.currentExpanded == index
    property alias percentUsage: freeSpaceBar.value
    property string freeSpaceText

    signal actionTriggered

    property alias actionIcon: action.iconName
    property alias actionToolTip: action.tooltip
    property bool actionVisible

    height: row.childrenRect.height + 2 * row.y
    hoverEnabled: true

    onContainsMouseChanged: {
        if (containsMouse) {
            devicenotifier.currentIndex = index
        }

        // this is done to hide the highlight if the mouse moves out of the list view
        // and we are not hovering anything
        if (deviceItem.ListView.view.highlightItem) {
            deviceItem.ListView.view.highlightItem.opacity = (containsMouse ? 1 : 0)
        }
    }

    onClicked: {
        var actions = hpSource.data[udi].actions
        if (actions.length === 1) {
            var service = hpSource.serviceForSource(udi)
            var operation = service.operationDescription("invokeAction")
            operation.predicate = actions[0].predicate
            service.startOperationCall(operation)
        } else {
            devicenotifier.currentExpanded = (expanded ? -1 : index)
        }
    }

    RowLayout {
        id: row
        anchors.horizontalCenter: parent.horizontalCenter
        y: units.smallSpacing
        width: parent.width - 2 * units.smallSpacing
        spacing: units.smallSpacing

        // FIXME: Device item loses focus on mounting/unmounting it,
        // or specifically, when some UI element changes.
        PlasmaCore.IconItem {
            id: deviceIcon
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: units.iconSizes.medium
            Layout.preferredHeight: width
            source: deviceItem.icon
            enabled: deviceItem.state == 0
            active: iconToolTip.containsMouse

            PlasmaCore.IconItem {
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                }
                id: emblem
                width: units.iconSizes.small
                height: width
                source: deviceItem.state == 0 ? emblemIcon : null
            }

            PlasmaCore.ToolTipArea {
                id: iconToolTip
                anchors.fill: parent
                subText: {
                    if ((mounted || deviceItem.state != 0) && model["Available Content"] != "Audio") {
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
        }

        Column {
            Layout.fillWidth: true

            move: Transition {
                NumberAnimation { property: "y"; duration: units.longDuration; easing.type: Easing.InOutQuad }
            }

            add: Transition {
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }

            PlasmaComponents.Label {
                id: deviceLabel
                width: parent.width
                height: undefined // reset PlasmaComponent.Label's strange default height
                elide: Text.ElideRight
            }

            PlasmaComponents.ProgressBar {
                id: freeSpaceBar
                width: parent.width
                height: units.gridUnit // default is * 1.6
                visible: deviceItem.state == 0 && mounted
                minimumValue: 0
                maximumValue: 100

                PlasmaCore.ToolTipArea {
                    anchors.fill: parent
                    subText: freeSpaceText != "" ? i18nc("@info:status Free disk space", "%1 free", freeSpaceText) : ""
                }

                // ProgressBar eats click events, so we'll forward them manually here...
                // setting enabled to false will also make the ProgressBar *look* disabled
                MouseArea {
                    anchors.fill: parent
                    onClicked: deviceItem.clicked(mouse)
                }
            }

            PlasmaComponents.Label {
                width: parent.width
                height: undefined
                opacity: 0.6
                font.pointSize: theme.smallestFont.pointSize
                visible: deviceItem.state != 0 || !actionsList.visible
                text: {
                    // FIXME: state changes do not reach the plasmoid if the
                    // device was already attached when the plasmoid was initialized
                    if (deviceItem.state == 0) {
                        if (!hpSource.data[udi]) {
                            return ""
                        }

                        var actions = hpSource.data[udi].actions
                        if (actions.length > 1) {
                            return i18np("1 action for this device", "%1 actions for this device", actions.length);
                        } else {
                            return actions[0].text
                        }
                    } else if (deviceItem.state == 1) {
                        return i18nc("Accessing is a less technical word for Mounting; translation should be short and mean \'Currently mounting this device\'", "Accessing...")
                    } else {
                        return i18nc("Removing is a less technical word for Unmounting; translation shoud be short and mean \'Currently unmounting this device\'", "Removing...")
                    }
                }
            }

            Item { // spacer
                width: 1
                height: units.smallSpacing
                visible: actionsList.visible
            }

            ListView {
                id: actionsList
                width: parent.width
                interactive: false
                model: hpSource.data[udi] ? hpSource.data[udi].actions : null
                height: deviceItem.expanded ? actionsList.contentHeight : 0
                visible: height > 0
                cacheBuffer: 50000 // create all items
                delegate: ActionItem {
                    width: actionsList.width
                    icon: modelData.icon
                    label: modelData.text
                    predicate: modelData.predicate
                }
                highlight: PlasmaComponents.Highlight {}
                highlightMoveDuration: 0
                highlightResizeDuration: 0
            }
        }

        Item {
            Layout.preferredWidth: units.iconSizes.medium
            Layout.fillHeight: true

            PlasmaComponents.ToolButton {
                id: action
                visible: !busyIndicator.visible && deviceItem.actionVisible
                onClicked: actionTriggered()
            }

            PlasmaComponents.BusyIndicator {
                id: busyIndicator
                width: parent.width
                height: width
                running: visible
                visible: deviceItem.state != 0
            }
        }
    }
}
