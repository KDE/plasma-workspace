/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2012 Jacopo De Simoi <wilderkde@gmail.com>
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2014 Marco Martin <mart@kde.org>
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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

MouseArea {
    Layout.minimumWidth: units.gridUnit * 18
    Layout.minimumHeight: units.gridUnit * 22

    hoverEnabled: true

    PlasmaCore.Svg {
        id: lineSvg
        imagePath: "widgets/line"
    }

    PlasmaExtras.Heading {
        width: parent.width
        level: 3
        opacity: 0.6
        text: i18n("No Devices Available")
        visible: filterModel.count == 0
    }

    PlasmaCore.DataSource {
        id: statusSource
        engine: "devicenotifications"
        property string last
        onSourceAdded: {
            console.debug("Source added " + last);
            last = source;
            disconnectSource(source);
            connectSource(source);
        }
        onSourceRemoved: {
            console.debug("Source removed " + last);
            disconnectSource(source);
        }
        onDataChanged: {
            console.debug("Data changed for " + last);
            console.debug("Error:" + data[last]["error"]);
            if (last != "") {
                statusBar.setData(data[last]["error"], data[last]["errorDetails"], data[last]["udi"]);
                plasmoid.expanded = true;
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        PlasmaExtras.ScrollArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: notifierDialog
                focus: true
                boundsBehavior: Flickable.StopAtBounds

                model: filterModel

                property int currentExpanded: -1
                property bool itemClicked: true
                delegate: deviceItem
                highlight: PlasmaComponents.Highlight { }
                highlightMoveDuration: 0
                highlightResizeDuration: 0

                //this is needed to make SectionScroller actually work
                //acceptable since one doesn't have a billion of devices
                cacheBuffer: 1000

                onCountChanged: {
                    if (count == 0) {
                        passiveTimer.restart()
                    } else {
                        passiveTimer.stop()
                        plasmoid.status = PlasmaCore.Types.ActiveStatus
                    }
                }

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
            }
        }

        PlasmaCore.SvgItem {
            id: statusBarSeparator
            Layout.fillWidth: true
            svg: lineSvg
            elementId: "horizontal-line"
            height: lineSvg.elementSize("horizontal-line").height

            visible: statusBar.height>0
            anchors.bottom: statusBar.top
        }

        StatusBar {
            id: statusBar
            Layout.fillWidth: true
            anchors.bottom: parent.bottom
        }
    }

    Component {
        id: deviceItem

        DeviceItem {
            id: wrapper
            width: notifierDialog.width
            udi: DataEngineSource
            icon: sdSource.data[udi] ? sdSource.data[udi]["Icon"] : ""
            deviceName: sdSource.data[udi] ? sdSource.data[udi]["Description"] : ""
            emblemIcon: Emblems[0]
            state: sdSource.data[udi]["State"]

            percentUsage: {
                if (!sdSource.data[udi]) {
                    return 0
                }
                var freeSpace = new Number(sdSource.data[udi]["Free Space"]);
                var size = new Number(model["Size"]);
                var used = size-freeSpace;
                return used*100/size;
            }
            freeSpaceText: sdSource.data[udi] && sdSource.data[udi]["Free Space Text"] ? sdSource.data[udi]["Free Space Text"] : ""

            leftActionIcon: {
                if (mounted) {
                    return "media-eject";
                } else {
                    return "emblem-mounted";
                }
            }
            mounted: devicenotifier.isMounted(udi)

            onLeftActionTriggered: {
                var operationName = mounted ? "unmount" : "mount";
                var service = sdSource.serviceForSource(udi);
                var operation = service.operationDescription(operationName);
                service.startOperationCall(operation);
            }
            property bool isLast: (expandedDevice == udi)
            property int operationResult: (model["Operation result"])

            onIsLastChanged: {
                if (isLast) {
                    devicenotifier.currentExpanded = index
                }
            }
            onOperationResultChanged: {
                if (operationResult == 1) {
                    devicenotifier.popupIcon = "dialog-ok"
                    popupIconTimer.restart()
                } else if (operationResult == 2) {
                    devicenotifier.popupIcon = "dialog-error"
                    popupIconTimer.restart()
                }
            }
            Behavior on height { NumberAnimation { duration: units.shortDuration * 3 } }
        }
    }
} // MouseArea
