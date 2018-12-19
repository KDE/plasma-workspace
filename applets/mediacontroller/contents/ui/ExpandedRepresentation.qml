/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *   Copyright 2014, 2016 Kai Uwe Broulik <kde@privat.broulik.de>          *
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

import QtQuick 2.4
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kcoreaddons 1.0 as KCoreAddons

Item {
    id: expandedRepresentation

    Layout.minimumWidth: Layout.minimumHeight * 1.333
    Layout.minimumHeight: units.gridUnit * 10
    Layout.preferredWidth: Layout.minimumWidth * 1.5
    Layout.preferredHeight: Layout.minimumHeight * 1.5

    readonly property int controlSize: units.iconSizes.large

    property double position: mpris2Source.currentData.Position || 0
    readonly property real rate: mpris2Source.currentData.Rate || 1
    readonly property double length: currentMetadata ? currentMetadata["mpris:length"] || 0 : 0
    readonly property bool canSeek: mpris2Source.currentData.CanSeek || false

    // only show hours (the default for KFormat) when track is actually longer than an hour
    readonly property int durationFormattingOptions: length >= 60*60*1000*1000 ? 0 : KCoreAddons.FormatTypes.FoldHours

    property bool disablePositionUpdate: false
    property bool keyPressed: false

    function retrievePosition() {
        var service = mpris2Source.serviceForSource(mpris2Source.current);
        var operation = service.operationDescription("GetPosition");
        service.startOperationCall(operation);
    }

    Connections {
        target: plasmoid
        onExpandedChanged: {
            if (plasmoid.expanded) {
                retrievePosition();
            }
        }
    }

    onPositionChanged: {
        // we don't want to interrupt the user dragging the slider
        if (!seekSlider.pressed && !keyPressed) {
            // we also don't want passive position updates
            disablePositionUpdate = true
            seekSlider.value = position
            disablePositionUpdate = false
        }
    }

    onLengthChanged: {
        disablePositionUpdate = true
        // When reducing maximumValue, value is clamped to it, however
        // when increasing it again it gets its old value back.
        // To keep us from seeking to the end of the track when moving
        // to a new track, we'll reset the value to zero and ask for the position again
        seekSlider.value = 0
        seekSlider.to = length
        retrievePosition()
        disablePositionUpdate = false
    }

    Keys.onPressed: keyPressed = true

    Keys.onReleased: {
        keyPressed = false

        if (!event.modifiers) {
            event.accepted = true

            if (event.key === Qt.Key_Space || event.key === Qt.Key_K) {
                // K is YouTube's key for "play/pause" :)
                root.togglePlaying()
            } else if (event.key === Qt.Key_P) {
                root.action_previous()
            } else if (event.key === Qt.Key_N) {
                root.action_next()
            } else if (event.key === Qt.Key_S) {
                root.action_stop()
            } else if (event.key === Qt.Key_Left || event.key === Qt.Key_J) { // TODO ltr languages
                // seek back 5s
                seekSlider.value = Math.max(0, seekSlider.value - 5000000) // microseconds
                seekSlider.moved();
            } else if (event.key === Qt.Key_Right || event.key === Qt.Key_L) {
                // seek forward 5s
                seekSlider.value = Math.min(seekSlider.to, seekSlider.value + 5000000)
                seekSlider.moved();
            } else if (event.key === Qt.Key_Home) {
                seekSlider.value = 0
                seekSlider.moved();
            } else if (event.key === Qt.Key_End) {
                seekSlider.value = seekSlider.to
                seekSlider.moved();
            } else if (event.key >= Qt.Key_0 && event.key <= Qt.Key_9) {
                // jump to percentage, ie. 0 = beginnign, 1 = 10% of total length etc
                seekSlider.value = seekSlider.to * (event.key - Qt.Key_0) / 10
                seekSlider.moved();
            } else {
                event.accepted = false
            }
        }
    }

    PlasmaComponents3.ComboBox {
        id: playerCombo
        width: Math.round(0.6 * parent.width)
        height: visible ? undefined : 0
        anchors.horizontalCenter: parent.horizontalCenter
        textRole: "text"
        visible: model.length > 2 // more than one player, @multiplex is always there
        model: {
            var model = [{
                text: i18n("Choose player automatically"),
                source: mpris2Source.multiplexSource
            }]

            var sources = mpris2Source.sources
            for (var i = 0, length = sources.length; i < length; ++i) {
                var source = sources[i]
                if (source === mpris2Source.multiplexSource) {
                    continue
                }

                // we could show the pretty player name ("Identity") here but then we
                // would have to connect all sources just for this
                model.push({text: source, source: source})
            }

            return model
        }

        onModelChanged: {
            // if model changes, ComboBox resets, so we try to find the current player again...
            for (var i = 0, length = model.length; i < length; ++i) {
                if (model[i].source === mpris2Source.current) {
                    currentIndex = i
                    break
                }
            }
        }

        onActivated: {
            disablePositionUpdate = true
            // ComboBox has currentIndex and currentText, why doesn't it have currentItem/currentModelValue?
            mpris2Source.current = model[index].source
            disablePositionUpdate = false
        }
    }

    Item {
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: playerCombo.bottom
            bottom: controlCol.top
            margins: units.smallSpacing
        }

        PlasmaCore.IconItem {
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }

            height: Math.round(parent.height / 2)
            width: height

            source: mpris2Source.currentData["Desktop Icon Name"]
            visible: !albumArt.visible

            usesPlasmaTheme: false
        }
    }

    Image {
        id: albumArt
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: playerCombo.bottom
            bottom: controlCol.top
            margins: units.smallSpacing
        }
        source: root.albumArt
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        sourceSize: Qt.size(height, height)
        visible: !!root.track && status === Image.Ready
    }

    Column {
        id: controlCol
        width: parent.width
        anchors.bottom: parent.bottom

        spacing: units.smallSpacing

        RowLayout {
            anchors {
                left: parent.left
                right: parent.right
                margins: units.smallSpacing
            }

            spacing: units.smallSpacing

            // if there's no "mpris:length" in the metadata, we cannot seek, so hide it in that case
            enabled: !root.noPlayer && root.track && expandedRepresentation.length > 0 ? true : false
            opacity: enabled ? 1 : 0
            Behavior on opacity {
                NumberAnimation { duration: units.longDuration }
            }

            // ensure the layout doesn't shift as the numbers change and measure roughly the longest text that could occur with the current song
            TextMetrics {
                id: timeMetrics
                text: i18nc("Remaining time for song e.g -5:42", "-%1",
                            KCoreAddons.Format.formatDuration(seekSlider.to / 1000, expandedRepresentation.durationFormattingOptions))
                font: theme.smallestFont
            }

            PlasmaComponents3.Label {
                Layout.preferredWidth: timeMetrics.width
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                text: KCoreAddons.Format.formatDuration(seekSlider.value / 1000, expandedRepresentation.durationFormattingOptions)
                opacity: 0.9
                font: theme.smallestFont
            }

            PlasmaComponents3.Slider {
                id: seekSlider
                Layout.fillWidth: true
                z: 999
                value: 0
                visible: canSeek

                onMoved: {
                    if (!disablePositionUpdate) {
                        // delay setting the position to avoid race conditions
                        queuedPositionUpdate.restart()
                    }
                }

                Timer {
                    id: seekTimer
                    interval: 1000 / expandedRepresentation.rate
                    repeat: true
                    running: root.state == "playing" && plasmoid.expanded && !keyPressed && interval > 0 && seekSlider.to >= 1000000
                    onTriggered: {
                        // some players don't continuously update the seek slider position via mpris
                        // add one second; value in microseconds
                        if (!seekSlider.pressed) {
                            disablePositionUpdate = true
                            if (seekSlider.value == seekSlider.to) {
                                retrievePosition();
                            } else {
                                seekSlider.value += 1000000
                            }
                            disablePositionUpdate = false
                        }
                    }
                }
            }

            PlasmaComponents3.ProgressBar {
                Layout.fillWidth: true
                value: seekSlider.value
                from: seekSlider.from
                to: seekSlider.to
                visible: !canSeek
            }

            PlasmaComponents3.Label {
                Layout.preferredWidth: timeMetrics.width
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                text: i18nc("Remaining time for song e.g -5:42", "-%1",
                            KCoreAddons.Format.formatDuration((seekSlider.to - seekSlider.value) / 1000, expandedRepresentation.durationFormattingOptions))
                opacity: 0.9
                font: theme.smallestFont
            }
        }

        Column {
            width: parent.width

            PlasmaExtras.Heading {
                id: song
                width: parent.width
                height: undefined
                level: 4
                horizontalAlignment: Text.AlignHCenter

                maximumLineCount: 1
                elide: Text.ElideRight
                text: {
                    if (!root.track) {
                        return i18n("No media playing")
                    }
                    return root.artist ? i18nc("artist – track", "%1 – %2", root.artist, root.track) : root.track
                }
                textFormat: Text.PlainText
            }

            PlasmaExtras.Heading {
                width: parent.width
                height: undefined
                level: 5
                opacity: 0.6
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                visible: text !== ""
                text: {
                    var metadata = root.currentMetadata
                    if (!metadata) {
                        return ""
                    }
                    var xesamAlbum = metadata["xesam:album"]
                    if (xesamAlbum) {
                        return xesamAlbum
                    }

                    // if we play a local file without title and artist, show its containing folder instead
                    if (metadata["xesam:title"] || root.artist) {
                        return ""
                    }

                    var xesamUrl = (metadata["xesam:url"] || "").toString()
                    if (xesamUrl.indexOf("file:///") !== 0) { // "!startsWith()"
                        return ""
                    }

                    var urlParts = xesamUrl.split("/")
                    if (urlParts.length < 3) {
                        return ""
                    }

                    var lastFolderPath = urlParts[urlParts.length - 2] // last would be filename
                    if (lastFolderPath) {
                        return lastFolderPath
                    }

                    return ""
                }
                textFormat: Text.PlainText
            }
        }

        Item {
            width: parent.width
            height: playerControls.height

            Row {
                id: playerControls
                property bool enabled: root.canControl
                property int controlsSize: theme.mSize(theme.defaultFont).height * 3

                anchors.horizontalCenter: parent.horizontalCenter
                spacing: units.largeSpacing

                PlasmaComponents3.ToolButton {
                    anchors.verticalCenter: parent.verticalCenter
                    width: expandedRepresentation.controlSize
                    height: width
                    enabled: playerControls.enabled && root.canGoPrevious
                    icon.name: LayoutMirroring.enabled ? "media-skip-forward" : "media-skip-backward"
                    onClicked: {
                        seekSlider.value = 0    // Let the media start from beginning. Bug 362473
                        root.action_previous()
                    }
                }

                PlasmaComponents3.ToolButton {
                    width: Math.round(expandedRepresentation.controlSize * 1.5)
                    height: width
                    enabled: root.state == "playing" ? root.canPause : root.canPlay
                    icon.name: root.state == "playing" ? "media-playback-pause" : "media-playback-start"
                    onClicked: root.togglePlaying()
                }

                PlasmaComponents3.ToolButton {
                    anchors.verticalCenter: parent.verticalCenter
                    width: expandedRepresentation.controlSize
                    height: width
                    enabled: playerControls.enabled && root.canGoNext
                    icon.name: LayoutMirroring.enabled ? "media-skip-backward" : "media-skip-forward"
                    onClicked: {
                        seekSlider.value = 0    // Let the media start from beginning. Bug 362473
                        root.action_next()
                    }
                }
            }
        }
    }

    Timer {
        id: queuedPositionUpdate
        interval: 100
        onTriggered: {
            if (position == seekSlider.value) {
                return;
            }
            var service = mpris2Source.serviceForSource(mpris2Source.current)
            var operation = service.operationDescription("SetPosition")
            operation.microseconds = seekSlider.value
            service.startOperationCall(operation)
        }
    }
}
