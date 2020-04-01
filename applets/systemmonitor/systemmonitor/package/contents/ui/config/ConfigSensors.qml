/*
 *   Copyright 2019 Marco Martin <mart@kde.org>
 *   Copyright 2019 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2019 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
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

import QtQuick 2.9
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as Controls
import QtQml.Models 2.12

import org.kde.kirigami 2.8 as Kirigami
import org.kde.kquickcontrols 2.0

import org.kde.kitemmodels 1.0 as KItemModels

import org.kde.ksysguard.sensors 1.0 as Sensors

import org.kde.plasma.core 2.1 as PlasmaCore

ColumnLayout {
    id: root

    readonly property int implicitHeight: 1 //HACK FIXME to disable external scrollbar

    property string cfg_totalSensor
    property alias cfg_sensorIds: usedSensorsView.sensorIds
    property alias cfg_sensorColors: usedSensorsView.sensorColors

    property alias cfg_textOnlySensorIds: textOnlySensorsView.sensorIds

    Sensors.Sensor {
        id: totalSensor
        sensorId: cfg_totalSensor
    }

    Component.onCompleted: {
        cfg_sensorIds = plasmoid.configuration.sensorIds
        cfg_sensorColors = plasmoid.configuration.sensorColors
        usedSensorsView.load();

        cfg_textOnlySensorIds = plasmoid.configuration.textOnlySensorIds
        textOnlySensorsView.load();
    }

    Component {
        id: delegateComponent
        Kirigami.SwipeListItem {
            id: listItem
            width: usedSensorsView.width
            actions: Kirigami.Action {
                icon.name: "list-remove"
                text: i18n("Remove")
                onTriggered: {
                    usedSensorsModel.remove(index, 1);
                    usedSensorsModel.save();
                }
            }
            contentItem: RowLayout {
                Kirigami.ListItemDragHandle {
                    listItem: listItem
                    listView: usedSensorsView
                    onMoveRequested: {
                        usedSensorsModel.move(oldIndex, newIndex, 1)
                        usedSensorsModel.save();
                    }
                }
                ColorButton {
                    id: textColorButton
                    color: model.color
                    onColorChanged: {
                        usedSensorsModel.setProperty(index, "color", color.toString());
                        usedSensorsModel.save();
                    }
                }
                Controls.Label {
                    Layout.fillWidth: true
                    text: sensor.name
                    Sensors.Sensor {
                        id: sensor
                        sensorId: model.sensorId
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.preferredHeight: sensorListHeader.implicitHeight
        visible: plasmoid.nativeInterface.supportsTotalSensor
        Controls.Label {
            text: i18n("Total Sensor:")
        }
        Controls.Label {
            Layout.fillWidth: true
            text: cfg_totalSensor.length > 0 ? totalSensor.name : i18n("Drop Sensor Here")
            elide: Text.ElideRight
            DropArea {
                anchors.fill: parent
                onEntered: {
                    if (drag.formats.indexOf("application/x-ksysguard") == -1) {
                        drag.accepted = false;
                        return;
                    }
                }
                onDropped: {
                    cfg_totalSensor =  drop.getDataAsString("application/x-ksysguard")
                }
            }
        }
        Controls.ToolButton {
            icon.name: "list-remove"
            opacity: cfg_totalSensor.length > 0
            onClicked: cfg_totalSensor = "";
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
        Layout.preferredHeight: 0

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 0
            Layout.preferredWidth: Kirigami.Units.gridUnit * 14

            Kirigami.Heading {
                Layout.preferredHeight: sensorListHeader.implicitHeight
                level: 3
                text: i18n("Chart Sensors")
            }
            UsedSensorsView {
                id: usedSensorsView
                showColor: plasmoid.nativeInterface.supportsSensorsColors
            }
            Kirigami.Heading {
                Layout.preferredHeight: sensorListHeader.implicitHeight
                text: i18n("Text Only Sensors")
                level: 3
                visible: textOnlySensorsView.visible
            }
            UsedSensorsView {
                id: textOnlySensorsView
                visible: plasmoid.nativeInterface.supportsTextOnlySensors
                showColor: false
            }
        }

        ColumnLayout {
            RowLayout {
                id: sensorListHeader
                Layout.fillWidth: true
                Controls.ToolButton {
                    icon.name: "go-previous"
                    enabled: sensorsDelegateModel.rootIndex.valid
                    onClicked: sensorsDelegateModel.rootIndex = sensorsDelegateModel.parentModelIndex()
                }
                Kirigami.Heading {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    level: 3
                    text: i18n("All Sensors")
                }
            }
            Kirigami.SearchField {
                id: searchQuery
                Layout.fillWidth: true
            }
            Controls.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 14

                ListView {

                    PlasmaCore.SortFilterModel {
                        id: sensorsSearchableModel
                        filterCaseSensitivity: Qt.CaseInsensitive
                        filterString: searchQuery.text
                        sourceModel: PlasmaCore.SortFilterModel {
                            filterRole: "SensorId"
                            filterCallback: function(row, value) {
                                return (value && value.length)
                            }
                            sourceModel: KItemModels.KDescendantsProxyModel {
                                model: allSensorsTreeModel
                            }
                        }
                    }

                    Sensors.SensorTreeModel {
                        id: allSensorsTreeModel
                    }

                    model: DelegateModel {
                        id: sensorsDelegateModel

                        model: searchQuery.text.length == 0 ?  allSensorsTreeModel : sensorsSearchableModel

                        delegate: Kirigami.BasicListItem {
                            id: sensorTreeDelegate
                            text: model.display
                            icon: (model.SensorId.length == 0) ? "folder" : ""

                            Drag.active: model.SensorId.length > 0 && sensorTreeDelegate.pressed
                            Drag.dragType: Drag.Automatic
                            Drag.supportedActions: Qt.CopyAction
                            Drag.hotSpot.x: sensorTreeDelegate.pressX
                            Drag.hotSpot.y: sensorTreeDelegate.pressY
                            Drag.mimeData: {
                                "application/x-ksysguard": model.SensorId
                            }
                            //FIXME: better handling of Drag
                            onPressed: {
                                onPressed: grabToImage(function(result) {
                                    Drag.imageSource = result.url
                                })
                            }
                            onClicked: {
                                if (model.SensorId.length == 0) {
                                    sensorsDelegateModel.rootIndex = sensorsDelegateModel.modelIndex(index);
                                }
                            }
                            onDoubleClicked: {
                                if (model.SensorId) {
                                    usedSensorsView.appendSensor(model.SensorId);
                                    usedSensorsView.positionViewAtIndex(usedSensorsView.count - 1, ListView.Contain);
                                }
                            }
                        }
                    }
                }
                Component.onCompleted: background.visible = true;
                Controls.ScrollBar.horizontal.visible: false
            }
        }
    }
}
