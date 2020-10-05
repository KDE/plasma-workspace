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

    header: PlasmaExtras.PlasmoidHeading {
        id: headerToolbarContainer

        RowLayout {
            width: parent.width

            PlasmaComponents3.Label {
                text: i18n("Show:")
            }

            PlasmaComponents3.ComboBox {
                id: displayModeCombobox

                Layout.preferredWidth: units.gridUnit * 11
                model: [i18n("Removable devices"),
                        i18n("Non-removable devices"),
                        i18n("All devices")
                       ]
                currentIndex: {
                    if (plasmoid.configuration.removableDevices) {
                        return 0
                    } else if (plasmoid.configuration.nonRemovableDevices) {
                        return 1
                    } else {
                        return 2
                    }
                }
                onActivated: {
                    switch (currentIndex) {
                        case 0:
                            plasmoid.configuration.removableDevices = true;
                            plasmoid.configuration.nonRemovableDevices = false;
                            plasmoid.configuration.allDevices = false;
                            break;
                        case 1:
                            plasmoid.configuration.removableDevices = false;
                            plasmoid.configuration.nonRemovableDevices = true;
                            plasmoid.configuration.allDevices = false;
                            break;
                        case 2:
                            plasmoid.configuration.removableDevices = false;
                            plasmoid.configuration.nonRemovableDevices = false;
                            plasmoid.configuration.allDevices = true;
                            break
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            PlasmaComponents3.ToolButton {
                id: unmountAll
                visible: devicenotifier.mountedRemovables > 1
                && displayModeCombobox.currentIndex === 0 /* removables only */

                icon.name: "media-eject"
                text: i18n("Remove All")

                PlasmaComponents3.ToolTip {
                    text: i18n("Click to safely remove all devices")
                }
            }

            // TODO: Once the automounter KCM is ported to QML, embed it in the
            // config window and change the action to open the config window
            PlasmaComponents3.ToolButton {
                icon.name: "configure"
                onClicked: plasmoid.action("openAutomounterKcm").trigger()
                visible: devicenotifier.openAutomounterKcmAuthorized

                Accessible.name: plasmoid.action("openAutomounterKcm").text

                PlasmaComponents3.ToolTip {
                    text: plasmoid.action("openAutomounterKcm").text
                }
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
            onLastChanged: {
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
        onExpandedChanged: {
            if (!plasmoid.expanded) {
                statusSource.clearMessage()
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
                text: i18n("No devices available")
                visible: notifierDialog.count === 0 && !devicenotifier.pendingDelegateRemoval
            }
        }
    }
}
