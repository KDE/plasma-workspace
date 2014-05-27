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

    ColumnLayout {
        anchors.fill: parent

        PlasmaComponents.Label {
            id: header
            text: filterModel.count>0 ? i18n("Available Devices") : i18n("No Devices Available")
            Layout.fillWidth: true
        }

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
                highlight: PlasmaComponents.Highlight{

                }

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
                        PlasmaCore.SvgItem {
                            visible: parent.y > 0
                            svg: lineSvg
                            elementId: "horizontal-line"
                            anchors {
                                left: parent.left
                                right: parent.right
                            }
                            height: lineSvg.elementSize("horizontal-line").height
                        }
                        PlasmaComponents.Label {
                            x: 8
                            y: 8
                            enabled: false
                            text: section
                            color: theme.textColor
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
        }

        StatusBar {
            id: statusBar
            Layout.fillWidth: true
        }
    }



    Component {
        id: deviceItem

        DeviceItem {
            id: wrapper
            width: notifierDialog.width
            udi: DataEngineSource
            icon: sdSource.data[udi]["Icon"]
            deviceName: sdSource.data[udi]["Description"]
            emblemIcon: Emblems[0]
            state: model["State"]

            percentUsage: {
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
            mounted: model["Accessible"]

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
                    plasmoid.setPopupIconByName("dialog-ok")
                    popupIconTimer.restart()
                } else if (operationResult == 2) {
                    plasmoid.setPopupIconByName("dialog-error")
                    popupIconTimer.restart()
                }
            }
            Behavior on height { NumberAnimation { duration: units.shortDuration * 3 } }
        }
    }
} // MouseArea
