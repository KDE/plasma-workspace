/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2012 Jacopo De Simoi <wilderkde@gmail.com>
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2014 Marco Martin <mart@kde.org>
 *   Copyright 2020 Nate Graham <nate@kde.org>
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

import QtQuick 2.2
import QtQuick.Window 2.2
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents // For Highlight
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

PlasmaComponents3.Page {
    id: fullRep
    property bool spontaneousOpen: false

    Layout.minimumWidth: units.gridUnit * 12
    Layout.minimumHeight: units.gridUnit * 12

    header: PlasmaExtras.BasicPlasmoidHeading {
        visible: !(plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading) || devicenotifier.mountedRemovables > 1
        PlasmaComponents3.ToolButton {
            id: unmountAll
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
            duration: 3000
            easing.type: Easing.InOutQuad
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

    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        ListView {
            id: notifierDialog
            focus: true
            boundsBehavior: Flickable.StopAtBounds

            model: filterModel

            delegate: DeviceItem {
                udi: DataEngineSource
            }
            highlight: PlasmaComponents.Highlight { }

            currentIndex: devicenotifier.currentIndex

            //this is needed to make SectionScroller actually work
            //acceptable since one doesn't have a billion of devices
            cacheBuffer: 1000

            section {
                property: "Type Description"
                delegate: Item {
                    height: childrenRect.height
                    width: notifierDialog.width
                    PlasmaExtras.Heading {
                        level: 3
                        opacity: 0.6
                        text: section
                    }
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - (units.largeSpacing * 4)
                text: plasmoid.configuration.removableDevices ? i18n("No removable devices attached") : i18n("No disks available")
                visible: notifierDialog.count === 0 && !devicenotifier.pendingDelegateRemoval
            }
        }
    }
}
