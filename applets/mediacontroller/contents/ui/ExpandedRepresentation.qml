/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

ColumnLayout {
    id: expandedRepresentation

    Layout.minimumWidth: Layout.minimumHeight * 1.333
    Layout.minimumHeight: theme.mSize(theme.defaultFont).height * 8
    Layout.preferredWidth: Layout.minimumWidth * 1.5
    Layout.preferredHeight: Layout.minimumHeight * 1.5

    property int controlSize: Math.min(height, width) / 4

    property int position: mpris2Source.data[mpris2Source.last].Position

    property bool isExpanded: plasmoid.expanded

    onIsExpandedChanged: {
        if (isExpanded) {
            var service = mpris2Source.serviceForSource(mpris2Source.last);
            var operation = service.operationDescription("GetPosition");
            service.startOperationCall(operation);
        }
    }

    onPositionChanged: {
        // don't set the position while the slider is pressed
        // which means the user is still holding it down
        if (!seekSlider.pressed) {
            seekSlider.value = position;
        }
    }

    //anchors.margins: units.largeSpacing
    //Rectangle { color: "orange"; anchors.fill: parent }

    RowLayout {
        id: titleRow
        spacing: units.largeSpacing
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
        }
        Image {
            source: mpris2Source.data[mpris2Source.last].Metadata["mpris:artUrl"]
            Layout.preferredHeight: Math.min(expandedRepresentation.height/2, sourceSize.height)
            Layout.preferredWidth: Layout.preferredHeight
            visible: status == Image.Ready && root.state != "off" && !root.noPlayer && mpris2Source.data[mpris2Source.last].Metadata["mpris:artUrl"] != undefined
        }
        Column {
            Layout.fillWidth: true
            anchors.top: parent.top
            spacing: units.largeSpacing
            PlasmaExtras.Heading {
                id: song
                anchors {
                    left: parent.left
                    right: parent.right
                }
                level: 3
                opacity: 0.6

                elide: Text.ElideRight
                text: root.track == "" ? i18n("No media playing") : root.track
            }

            PlasmaExtras.Heading {
                id: artist
                anchors {
                    left: parent.left
                    right: parent.right
                }
                level: 4
                opacity: 0.3

                elide: Text.ElideRight
                text: root.noPlayer ? "" : root.artist
            }
        }
    }

    PlasmaComponents.Slider {
        id: seekSlider
        z: 999
        maximumValue: mpris2Source.data[mpris2Source.last].Metadata["mpris:length"]
        value: 0
        // if there's no "mpris:length" in teh metadata, we cannot seek, so hide it in that case
        visible: root.state != "off" && !root.noPlayer && mpris2Source.data[mpris2Source.last].Metadata["mpris:length"] != undefined
        anchors {
            left: parent.left
            right: parent.right
        }
        onValueChanged: {
            if (pressed) {
                var service = mpris2Source.serviceForSource(mpris2Source.last);
                var operation = service.operationDescription("SetPosition");
                operation.microseconds = value
                service.startOperationCall(operation);
            }
        }

        Timer {
            id: seekTimer
            interval: 1000
            repeat: true
            running: root.state == "playing" && plasmoid.expanded
            onTriggered: {
                // add one second; value in microseconds
                seekSlider.value += 1000000;
            }
        }
    }

    Item {
        Layout.fillHeight: true
        anchors {
            left: parent.left
            right: parent.right
        }


        Row {
            id: playerControls
            property int controlsSize: theme.mSize(theme.defaultFont).height * 3

            //Rectangle { color: "orange"; anchors.fill: parent }

            anchors.centerIn: parent
            spacing: units.largeSpacing

            MediaControl {
                anchors.verticalCenter: parent.verticalCenter
                source: "media-skip-backward"
                onTriggered: root.previous();
            }

            MediaControl {
                width: expandedRepresentation.controlSize * 1.5
                source: root.state == "playing" ? "media-playback-pause" : "media-playback-start"
                onTriggered: {
                    print("Clicked" + source + " " + root.state);
                    if (root.state == "playing") {
                        root.pause();
                    } else {
                        root.play();
                    }
                }
            }

            MediaControl {
                anchors.verticalCenter: parent.verticalCenter
                source: "media-skip-forward"
                onTriggered: root.next();
            }
        }
    }
}