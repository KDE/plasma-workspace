/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *   Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>                *
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

Item {
    id: expandedRepresentation

    Layout.minimumWidth: Layout.minimumHeight * 1.333
    Layout.minimumHeight: theme.mSize(theme.defaultFont).height * 8
    Layout.preferredWidth: Layout.minimumWidth * 1.5
    Layout.preferredHeight: Layout.minimumHeight * 1.5

    readonly property int controlSize: Math.min(height, width) / 4
    // Basically just needed to match the right margin to the left in systray popup
    readonly property bool constrained: plasmoid.formFactor == PlasmaCore.Types.Vertical || plasmoid.formFactor == PlasmaCore.Types.Horizontal

    property int position: mpris2Source.data[mpris2Source.current].Position
    property bool disablePositionUpdate: false

    property bool isExpanded: plasmoid.expanded

    onIsExpandedChanged: {
        if (isExpanded) {
            var service = mpris2Source.serviceForSource(mpris2Source.current);
            var operation = service.operationDescription("GetPosition");
            service.startOperationCall(operation);
        }
    }

    onPositionChanged: {
        // we don't want to interrupt the user dragging the slider
        if (!seekSlider.pressed) {
            // we also don't want passive position updates
            disablePositionUpdate = true
            seekSlider.value = position
            disablePositionUpdate = false
        }
    }

    Column {
        id: titleColumn
        width: constrained ? parent.width - units.largeSpacing : parent.width
        spacing: units.smallSpacing

        RowLayout {
            id: titleRow
            spacing: units.largeSpacing
            width: parent.width

            Image {
                id: albumArt
                source: root.albumArt
                fillMode: Image.PreserveAspectCrop
                Layout.preferredHeight: expandedRepresentation.height / 2
                Layout.preferredWidth: Layout.preferredHeight
                visible: !!root.track

                PlasmaCore.IconItem {
                    anchors.fill: parent
                    source: "tools-rip-audio-cd" // FIXME VDG Needs a proper album art cover dummy
                    visible: parent.status !== Image.Ready
                }
            }

            Column {
                Layout.fillWidth: true
                spacing: units.smallSpacing / 2

                PlasmaExtras.Heading {
                    id: song
                    width: parent.width
                    level: 3
                    opacity: 0.6

                    elide: Text.ElideRight
                    text: root.track ? root.track : i18n("No media playing")
                }

                PlasmaExtras.Heading {
                    id: artist
                    width: parent.width
                    level: 4
                    opacity: 0.4

                    elide: Text.ElideRight
                    text: root.artist ? root.artist : ""
                }
            }
        }

        PlasmaComponents.Slider {
            id: seekSlider
            width: parent.width
            z: 999
            maximumValue: currentMetadata ? currentMetadata["mpris:length"] || 0 : 0
            value: 0
            // if there's no "mpris:length" in teh metadata, we cannot seek, so hide it in that case
            enabled: !root.noPlayer && root.track && currentMetadata && currentMetadata["mpris:length"] && mpris2Source.data[mpris2Source.current].CanSeek
            opacity: enabled ? 1 : 0
            Behavior on opacity {
                NumberAnimation { duration: units.longDuration }
            }

            onValueChanged: {
                if (!disablePositionUpdate) {
                    // delay setting the position to avoid race conditions
                    queuedPositionUpdate.restart()
                }
            }

            Timer {
                id: seekTimer
                interval: 1000
                repeat: true
                running: root.state == "playing" && plasmoid.expanded
                onTriggered: {
                    // some players don't continuously update the seek slider position via mpris
                    // add one second; value in microseconds
                    if (!seekSlider.pressed) {
                        disablePositionUpdate = true
                        seekSlider.value += 1000000
                        disablePositionUpdate = false
                    }
                }
            }
        }
    }

    Timer {
        id: queuedPositionUpdate
        interval: 100
        onTriggered: {
            var service = mpris2Source.serviceForSource(mpris2Source.current)
            var operation = service.operationDescription("SetPosition")
            operation.microseconds = seekSlider.value
            service.startOperationCall(operation)
        }
    }

    PlasmaComponents.Button {
        anchors {
            right: titleColumn.right
            bottom: titleColumn.bottom
            bottomMargin: seekSlider.height // Cannot anchor around in a column/row, and being lazy
        }
        text: i18nc("Bring the window of player %1 to the front", "Open %1", mpris2Source.data[mpris2Source.current].Identity)
        visible: !root.noPlayer && mpris2Source.data[mpris2Source.current].CanRaise
        onClicked: root.action_openplayer()
    }

    Item {
        anchors.bottom: parent.bottom
        width: constrained ? parent.width - units.largeSpacing : parent.width
        height: playerControls.height

        Row {
            id: playerControls
            property bool enabled: !root.noPlayer && mpris2Source.data[mpris2Source.current].CanControl
            property int controlsSize: theme.mSize(theme.defaultFont).height * 3

            anchors.horizontalCenter: parent.horizontalCenter
            spacing: units.largeSpacing

            PlasmaComponents.ToolButton {
                anchors.verticalCenter: parent.verticalCenter
                width: expandedRepresentation.controlSize
                height: width
                enabled: playerControls.enabled && mpris2Source.data[mpris2Source.current].CanGoPrevious
                iconSource: "media-skip-backward"
                onClicked: root.previous()
            }

            PlasmaComponents.ToolButton {
                width: expandedRepresentation.controlSize * 1.5
                height: width
                enabled: playerControls.enabled
                iconSource: root.state == "playing" ? "media-playback-pause" : "media-playback-start"
                onClicked: root.playPause()
            }

            PlasmaComponents.ToolButton {
                anchors.verticalCenter: parent.verticalCenter
                width: expandedRepresentation.controlSize
                height: width
                enabled: playerControls.enabled && mpris2Source.data[mpris2Source.current].CanGoNext
                iconSource: "media-skip-forward"
                onClicked: root.next()
            }
        }
    }
}
