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

import org.kde.kirigami 2.5 as Kirigami
import org.kde.kquickcontrols 2.0

import org.kde.ksysguard.sensors 1.0 as Sensors

DropArea {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.preferredWidth: usedSensorsScroll.implicitWidth

    property alias count: usedSensorsView.count
    property alias delegateComponent: delegateComponent
    property var sensorIds: []
    property var sensorColors: []
    property bool showColor: true

    function appendSensor(sensorId) {
        insertSensor(usedSensorsModel.count, sensorId);
    }

    function insertSensor(index, sensorId) {
        var color = Kirigami.Theme.highlightColor;
        color.hsvHue = Math.random();
        usedSensorsModel.insert(index, {"sensorId": sensorId, "color": color.toString()})
        usedSensorsModel.save()
    }

    function positionViewAtIndex(index, mode) {
        usedSensorsView.positionViewAtIndex(index, mode);
    }

    function load() {
        for (var i in sensorIds) {
            usedSensorsModel.append({"sensorId": sensorIds[i], "color": (sensorColors[i] || "").toString()})
        }
    }
    Component {
        id: delegateComponent
        Kirigami.SwipeListItem {
            id: listItem
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
                    visible: root.showColor
                    showAlphaChannel: true
                    onAccepted: {
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

    onEntered: {
        if (drag.formats.indexOf("application/x-ksysguard") == -1) {
            drag.accepted = false;
            return;
        }
    }
    function dropIndex() {
        if (!containsDrag) {
            return -1;
        } else if (usedSensorsView.count == 0) {
            return 0;
        }

        var itemHeight = usedSensorsView.contentItem.children[0].height;
        var contentY = usedSensorsView.contentY + drag.y;

        return Math.max(0, Math.min(usedSensorsView.count, Math.round(contentY / itemHeight)));
    }

    onDropped: insertSensor(dropIndex(), drop.getDataAsString("application/x-ksysguard"))

    Controls.Label {
        anchors.centerIn: parent
        z: 2
        visible: usedSensorsView.count == 0
        text: i18n("Drop Sensors Here")
    }
    Rectangle {
        anchors {
            left: parent.left
            right: parent.right
        }
        height: Kirigami.Units.smallSpacing
        color: Kirigami.Theme.highlightColor
        visible: parent.containsDrag
        y: {
            if (usedSensorsView.count == 0) {
                return 0;
            }
            var itemHeight = usedSensorsView.contentItem.children[0].height;
            return itemHeight * parent.dropIndex() - usedSensorsView.contentY;
        }
        opacity: 0.6
        z: 2
    }
    Controls.ScrollView {
        id: usedSensorsScroll
        anchors.fill: parent

        ListView {
            id: usedSensorsView

            model: ListModel {
                id: usedSensorsModel
                function save() {
                    var ids = [];
                    var colors = [];
                    for (var i = 0; i < count; ++i) {
                        ids.push(get(i).sensorId);
                        colors.push(get(i).color);
                    }
                    root.sensorIds = ids;
                    root.sensorColors = colors;
                }
            }
            //NOTE: this row is necessary to make the drag handle work
            delegate: Kirigami.DelegateRecycler {
                width: usedSensorsView.width
                sourceComponent: delegateComponent
            }
        }
        Component.onCompleted: background.visible = true;
        Controls.ScrollBar.horizontal.visible: false
    }
}

