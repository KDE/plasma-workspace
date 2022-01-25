/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.2
import QtQuick.Window 2.2
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents // For Highlight
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

PlasmaExtras.Representation {
    id: fullRep
    property bool spontaneousOpen: false

    Layout.minimumWidth: PlasmaCore.Units.gridUnit * 12
    Layout.minimumHeight: PlasmaCore.Units.gridUnit * 12

    collapseMarginsHint: true

    header: PlasmaExtras.PlasmoidHeading {
        visible: !(plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading) && devicenotifier.mountedRemovables > 1
        PlasmaComponents3.ToolButton {
            id: unmountAll
            anchors.right: parent.right
            visible: devicenotifier.mountedRemovables > 1;

            icon.name: "media-eject"
            text: i18n("Remove All")

            PlasmaComponents3.ToolTip {
                text: i18n("Click to safely remove all devices")
            }
        }
    }

    MouseArea {
        id: fullRepMouseArea
        hoverEnabled: true
    }

    PlasmaCore.DataSource {
        id: userActivitySource
        engine: "powermanagement"
        connectedSources: "UserActivity"
        property int polls: 0
        //poll only on plasmoid expanded
        interval: !fullRepMouseArea.containsMouse && !fullRep.Window.active && spontaneousOpen && plasmoid.expanded ? 3000 : 0
        onIntervalChanged: polls = 0;
        onDataChanged: {
            //only do when polling
            if (interval == 0 || polls++ < 1) {
                return;
            }

            if (userActivitySource.data["UserActivity"]["IdleTime"] < interval) {
                plasmoid.expanded = false;
                spontaneousOpen = false;
            }
        }
    }


    // this item is reparented to a delegate that is showing a message to draw focus to it
    PlasmaComponents.Highlight {
        id: messageHighlight
        visible: false

        OpacityAnimator {
            id: messageHighlightAnimator
            target: messageHighlight
            from: 1
            to: 0
            duration: PlasmaCore.Units.veryLongDuration * 8
            easing.type: Easing.InOutQuad
            Component.onCompleted: devicenotifier.isMessageHighlightAnimatorRunning = Qt.binding(() => running);
        }

        Connections {
            target: statusSource
            function onLastChanged() {
                if (!statusSource.last) {
                    messageHighlightAnimator.stop()
                    messageHighlight.visible = false
                }
            }
        }

        function highlight(item) {
            parent = item
            width = Qt.binding(function() { return item.width })
            height = Qt.binding(function() { return item.height })
            opacity = 1 // Animator is threaded so the old opacity might be visible for a frame or two
            visible = true
            messageHighlightAnimator.start()
        }
    }

    Connections {
        target: plasmoid
        function onExpandedChanged() {
            if (!plasmoid.expanded) {
                statusSource.clearMessage();
            }
        }
    }

    PlasmaComponents3.ScrollView {
        id: scrollView

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        anchors.fill: parent
        contentWidth: availableWidth - contentItem.leftMargin - contentItem.rightMargin

        contentItem: ListView {
            id: notifierDialog
            focus: true

            model: filterModel

            delegate: DeviceItem {
                width: ListView.view.width - PlasmaCore.Units.smallSpacing * 4
                udi: DataEngineSource
            }
            highlight: PlasmaComponents.Highlight { }
            highlightMoveDuration: 0
            highlightResizeDuration: 0

            topMargin: PlasmaCore.Units.smallSpacing * 2
            bottomMargin: PlasmaCore.Units.smallSpacing * 2
            leftMargin: PlasmaCore.Units.smallSpacing * 2
            rightMargin: PlasmaCore.Units.smallSpacing * 2
            spacing: PlasmaCore.Units.smallSpacing

            currentIndex: devicenotifier.currentIndex

            //this is needed to make SectionScroller actually work
            //acceptable since one doesn't have a billion of devices
            cacheBuffer: 1000

            // FIXME: the model is sorted by timestamp, not type, this results in sections possibly getting listed
            //   multiple times
            section {
                property: "Type Description"
                delegate: Item {
                    height: Math.floor(childrenRect.height)
                    width: notifierDialog.width - (scrollView.PlasmaComponents3.ScrollBar.vertical.visible ? PlasmaCore.Units.smallSpacing * 4 : 0)
                    PlasmaExtras.Heading {
                        level: 3
                        opacity: 0.6
                        text: section
                    }
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - (PlasmaCore.Units.largeSpacing * 4)
                text: plasmoid.configuration.removableDevices ? i18n("No removable devices attached") : i18n("No disks available")
                visible: notifierDialog.count === 0 && !messageHighlightAnimator.running
            }
        }
    }
}
