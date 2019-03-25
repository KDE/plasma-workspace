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
    property alias icon: deviceIcon.source
    property alias deviceName: deviceLabel.text
    property string emblemIcon
    property int state

    property bool mounted
    property bool isRoot
    property bool expanded: devicenotifier.expandedDevice == udi
    property alias percentUsage: freeSpaceBar.value
    property string freeSpaceText

    signal actionTriggered

    property alias actionIcon: actionButton.iconName
    property alias actionToolTip: actionButton.tooltip
    property bool actionVisible

    readonly property bool hasMessage: statusSource.lastUdi == udi && statusSource.data[statusSource.last] ? true : false
    readonly property var message: hasMessage ? statusSource.data[statusSource.last] || ({}) : ({})

    height: row.childrenRect.height + 2 * row.y
    hoverEnabled: true

    onHasMessageChanged: {
        if (hasMessage) {
            messageHighlight.highlight(this)
        }
    }

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
        var data = hpSource.data[udi]
        if (!data) {
            return
        }

        var actions = data.actions
        if (actions.length === 1) {
            var service = hpSource.serviceForSource(udi)
            var operation = service.operationDescription("invokeAction")
            operation.predicate = actions[0].predicate
            service.startOperationCall(operation)
        } else {
            devicenotifier.expandedDevice = (expanded ? "" : udi)
        }
    }

    Connections {
        target: unmountAll
        onClicked: {
            if (model["Removable"] && mounted) {
                actionTriggered();
            }
        }
    }

    // this keeps the delegate around for 5 seconds after the device has been
    // removed in case there was a message, such as "you can now safely remove this"
    ListView.onRemove: {
        if (devicenotifier.expandedDevice == udi) {
            devicenotifier.expandedDevice = ""
        }

        if (deviceItem.hasMessage) {
            ListView.delayRemove = true
            keepDelegateTimer.restart()

            statusMessage.opacity = 1 // HACK seems the Column animation breaksf
            freeSpaceBar.visible = false
            actionButton.visible = false

            ++devicenotifier.pendingDelegateRemoval // QTBUG-50380
        }
    }

    Timer {
        id: keepDelegateTimer
        interval: 3000 // same interval as the auto hide / passive timer
        onTriggered: {
            deviceItem.ListView.delayRemove = false
            // otherwise the last message will show again when this device reappears
            statusSource.clearMessage()

            --devicenotifier.pendingDelegateRemoval // QTBUG-50380
        }
    }

    Timer {
        id: updateStorageSpaceTimer
        interval: 5000
        repeat: true
        running: mounted && plasmoid.expanded
        triggeredOnStart: true     // Update the storage space as soon as we open the plasmoid
        onTriggered: {
            var service = sdSource.serviceForSource(udi);
            var operation = service.operationDescription("updateFreespace");
            service.startOperationCall(operation);
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
            enabled: deviceItem.state == 0
            active: iconToolTip.containsMouse

            PlasmaCore.IconItem {
                id: deviceEmblem
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                }
                width: units.iconSizes.small
                height: width
                source: {
                    if (deviceItem.hasMessage) {
                        if (deviceItem.message.solidError === 0) {
                            return "emblem-information"
                        } else {
                            return "emblem-error"
                        }
                    } else if (deviceItem.state == 0) {
                        return emblemIcon
                    } else {
                        return ""
                    }
                }
            }

            PlasmaCore.ToolTipArea {
                id: iconToolTip
                anchors.fill: parent
                subText: {
                    if ((mounted || deviceItem.state != 0) && model["Available Content"] !== "Audio") {
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
                // ensure opacity values return to 1.0 if the add transition animation has been interrupted
                NumberAnimation { property: "opacity"; to: 1.0 }
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
                id: actionMessage
                width: parent.width
                height: undefined
                opacity: 0.6
                font.pointSize: theme.smallestFont.pointSize
                visible: deviceItem.state != 0 || (!actionsList.visible && !deviceItem.hasMessage)
                text: {
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
                        return i18nc("Removing is a less technical word for Unmounting; translation should be short and mean \'Currently unmounting this device\'", "Removing...")
                    }
                }
            }

            PlasmaComponents.Label {
                id: statusMessage
                width: parent.width
                height: undefined
                font.pointSize: theme.smallestFont.pointSize
                text: deviceItem.hasMessage ? (deviceItem.message.error || "") : ""
                wrapMode: Text.WordWrap
                maximumLineCount: 10
                elide: Text.ElideRight
                visible: deviceItem.hasMessage
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
                id: actionButton
                visible: !busyIndicator.visible && deviceItem.actionVisible
                enabled: !isRoot
                onClicked: actionTriggered()
                y: mounted ? deviceLabel.height + (freeSpaceBar.height - height - units.smallSpacing) / 2 : (deviceLabel.height + actionMessage.height - height) / 2
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
