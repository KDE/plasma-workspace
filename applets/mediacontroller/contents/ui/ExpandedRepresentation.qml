/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *   Copyright 2014, 2016 Kai Uwe Broulik <kde@privat.broulik.de>          *
 *   Copyright 2020 Carson Black <uhhadd@gmail.com>                        *
 *   Copyright 2020 Ismael Asensio <isma.af@gmail.com>                     *
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

import QtQuick 2.8
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kcoreaddons 1.0 as KCoreAddons
import org.kde.kirigami 2.4 as Kirigami
import QtGraphicalEffects 1.0

PlasmaExtras.Representation {
    id: expandedRepresentation

    Layout.minimumWidth: PlasmaCore.Units.gridUnit * 14
    Layout.minimumHeight: PlasmaCore.Units.gridUnit * 14
    Layout.preferredWidth: Layout.minimumWidth * 1.5
    Layout.preferredHeight: Layout.minimumHeight * 1.5

    collapseMarginsHint: true

    readonly property int controlSize: PlasmaCore.Units.iconSizes.medium

    property double position: mpris2Source.currentData.Position || 0
    readonly property real rate: mpris2Source.currentData.Rate || 1
    readonly property double length: currentMetadata ? currentMetadata["mpris:length"] || 0 : 0
    readonly property bool canSeek: mpris2Source.currentData.CanSeek || false
    readonly property bool softwareRendering: GraphicsInfo.api === GraphicsInfo.Software

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
        function onExpandedChanged() {
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

    Item { // Album Art Background + Details
        anchors.fill: parent

        Image {
            id: backgroundImage

            source: root.albumArt
            sourceSize.width: 512 /*
                                    * Setting a sourceSize.width here
                                    * prevents flickering when resizing the
                                    * plasmoid on a desktop.
                                    */

            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop

            asynchronous: true
            visible: !!root.track && status === Image.Ready && !softwareRendering

            layer.enabled: !softwareRendering
            layer.effect: HueSaturation {
                cached: true

                lightness: -0.5
                saturation: 0.9

                layer.enabled: true
                layer.effect: GaussianBlur {
                    cached: true

                    radius: 128
                    deviation: 12
                    samples: 63

                    transparentBorder: false
                }
            }
        }
        RowLayout { // Album Art + Details
            id: albumRow

            anchors {
                fill: parent
                leftMargin: PlasmaCore.Units.largeSpacing
                rightMargin: PlasmaCore.Units.largeSpacing
            }

            spacing: PlasmaCore.Units.largeSpacing

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 50

                Image { // Album Art
                    id: albumArt

                    anchors.fill: parent

                    visible: !!root.track && status === Image.Ready

                    asynchronous: true

                    horizontalAlignment: Image.AlignRight
                    verticalAlignment: Image.AlignVCenter
                    fillMode: Image.PreserveAspectFit

                    source: root.albumArt
                }

                PlasmaCore.IconItem { // Fallback
                    visible: !albumArt.visible
                    source: {
                        if (mpris2Source.currentData["Desktop Icon Name"])
                            return mpris2Source.currentData["Desktop Icon Name"]
                        return "media-album-cover"
                    }

                    anchors {
                        fill: parent
                        margins: PlasmaCore.Units.largeSpacing*2
                    }
                }
            }

            ColumnLayout { // Details Column
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 50
                Layout.alignment: !(albumArt.visible || !!mpris2Source.currentData["Desktop Icon Name"]) ? Qt.AlignHCenter : 0

                /*
                    * We use Kirigami.Heading instead of PlasmaExtras.Heading
                    * to prevent a binding loop caused by the PC2 Label component
                    * used by PlasmaExtras.Heading
                    */
                Kirigami.Heading { // Song Title
                    id: songTitle
                    level: 1

                    color: (softwareRendering || !albumArt.visible) ? PlasmaCore.ColorScope.textColor : "white"

                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.VerticalFit
                    elide: Text.ElideRight

                    text: root.track || i18n("No media playing")

                    Layout.fillWidth: true
                    Layout.maximumHeight: PlasmaCore.Units.gridUnit*5
                }
                Kirigami.Heading { // Song Artist
                    id: songArtist
                    visible: root.track && root.artist
                    level: 2

                    color: (softwareRendering || !albumArt.visible) ? PlasmaCore.ColorScope.textColor : "white"

                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.VerticalFit
                    elide: Text.ElideRight

                    text: root.artist
                    Layout.fillWidth: true
                    Layout.maximumHeight: PlasmaCore.Units.gridUnit*2
                }
                Kirigami.Heading { // Song Album
                    color: (softwareRendering || !albumArt.visible) ? PlasmaCore.ColorScope.textColor : "white"

                    level: 3
                    opacity: 0.6

                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.VerticalFit
                    elide: Text.ElideRight

                    visible: text.length !== 0
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
                    Layout.fillWidth: true
                    Layout.maximumHeight: PlasmaCore.Units.gridUnit*2
                }
            }
        }
    }

    footer: PlasmaExtras.PlasmoidHeading {
        id: footerItem
        location: PlasmaExtras.PlasmoidHeading.Location.Footer
        ColumnLayout { // Main Column Layout
            anchors.fill: parent
            RowLayout { // Seek Bar
                spacing: PlasmaCore.Units.smallSpacing

                // if there's no "mpris:length" in the metadata, we cannot seek, so hide it in that case
                enabled: !root.noPlayer && root.track && expandedRepresentation.length > 0 ? true : false
                opacity: enabled ? 1 : 0
                Behavior on opacity {
                    NumberAnimation { duration: PlasmaCore.Units.longDuration }
                }

                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.maximumWidth: Math.min(PlasmaCore.Units.gridUnit*45, Math.round(expandedRepresentation.width*(7/10)))

                // ensure the layout doesn't shift as the numbers change and measure roughly the longest text that could occur with the current song
                TextMetrics {
                    id: timeMetrics
                    text: i18nc("Remaining time for song e.g -5:42", "-%1",
                                KCoreAddons.Format.formatDuration(seekSlider.to / 1000, expandedRepresentation.durationFormattingOptions))
                    font: PlasmaCore.Theme.smallestFont
                }

                PlasmaComponents3.Label { // Time Elapsed
                    Layout.preferredWidth: timeMetrics.width
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    text: KCoreAddons.Format.formatDuration(seekSlider.value / 1000, expandedRepresentation.durationFormattingOptions)
                    opacity: 0.9
                    font: PlasmaCore.Theme.smallestFont
                    color: PlasmaCore.ColorScope.textColor
                }

                PlasmaComponents3.Slider { // Slider
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
                        running: root.state === "playing" && plasmoid.expanded && !keyPressed && interval > 0 && seekSlider.to >= 1000000
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

                RowLayout {
                    visible: !canSeek

                    Layout.fillWidth: true
                    Layout.preferredHeight: seekSlider.height

                    PlasmaComponents3.ProgressBar { // Time Remaining
                        value: seekSlider.value
                        from: seekSlider.from
                        to: seekSlider.to

                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                PlasmaComponents3.Label {
                    Layout.preferredWidth: timeMetrics.width
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    text: i18nc("Remaining time for song e.g -5:42", "-%1",
                                KCoreAddons.Format.formatDuration((seekSlider.to - seekSlider.value) / 1000, expandedRepresentation.durationFormattingOptions))
                    opacity: 0.9
                    font: PlasmaCore.Theme.smallestFont
                    color: PlasmaCore.ColorScope.textColor
                }
            }

            RowLayout { // Player Controls
                id: playerControls

                property bool enabled: root.canControl
                property int controlsSize: PlasmaCore.Theme.mSize(PlasmaCore.Theme.defaultFont).height * 3

                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: PlasmaCore.Units.smallSpacing
                spacing: PlasmaCore.Units.smallSpacing

                PlasmaComponents3.ToolButton {
                    Layout.rightMargin: LayoutMirroring.enabled ? 0 : PlasmaCore.Units.largeSpacing - playerControls.spacing
                    Layout.leftMargin: LayoutMirroring.enabled ? PlasmaCore.Units.largeSpacing - playerControls.spacing : 0
                    icon.name: "media-playlist-shuffle"
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize
                    checked: root.shuffle === true
                    enabled: root.canControl && root.shuffle !== undefined
                    Accessible.name: i18n("Shuffle")
                    onClicked: {
                        const service = mpris2Source.serviceForSource(mpris2Source.current);
                        let operation = service.operationDescription("SetShuffle");
                        operation.on = !root.shuffle;
                        service.startOperationCall(operation);
                    }

                    PlasmaComponents3.ToolTip {
                        text: parent.Accessible.name
                    }
                }

                PlasmaComponents3.ToolButton { // Previous
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize
                    Layout.alignment: Qt.AlignVCenter
                    enabled: playerControls.enabled && root.canGoPrevious
                    icon.name: LayoutMirroring.enabled ? "media-skip-forward" : "media-skip-backward"
                    onClicked: {
                        seekSlider.value = 0    // Let the media start from beginning. Bug 362473
                        root.action_previous()
                    }
                }

                PlasmaComponents3.ToolButton { // Pause/Play
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize
                    Layout.alignment: Qt.AlignVCenter
                    enabled: root.state == "playing" ? root.canPause : root.canPlay
                    icon.name: root.state == "playing" ? "media-playback-pause" : "media-playback-start"
                    onClicked: root.togglePlaying()
                }

                PlasmaComponents3.ToolButton { // Next
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize
                    Layout.alignment: Qt.AlignVCenter
                    enabled: playerControls.enabled && root.canGoNext
                    icon.name: LayoutMirroring.enabled ? "media-skip-backward" : "media-skip-forward"
                    onClicked: {
                        seekSlider.value = 0    // Let the media start from beginning. Bug 362473
                        root.action_next()
                    }
                }

                PlasmaComponents3.ToolButton {
                    Layout.leftMargin: LayoutMirroring.enabled ? 0 : PlasmaCore.Units.largeSpacing - playerControls.spacing
                    Layout.rightMargin: LayoutMirroring.enabled ? PlasmaCore.Units.largeSpacing - playerControls.spacing : 0
                    icon.name: root.loopStatus === "Track" ? "media-playlist-repeat-song" : "media-playlist-repeat"
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize
                    checked: root.loopStatus !== undefined && root.loopStatus !== "None"
                    enabled: root.canControl && root.loopStatus !== undefined
                    Accessible.name: root.loopStatus === "Track" ? i18n("Repeat Track") : i18n("Repeat")
                    onClicked: {
                        const service = mpris2Source.serviceForSource(mpris2Source.current);
                        let operation = service.operationDescription("SetLoopStatus");
                        switch (root.loopStatus) {
                        case "Playlist":
                            operation.status = "Track";
                            break;
                        case "Track":
                            operation.status = "None";
                            break;
                        default:
                            operation.status = "Playlist";
                        }
                        service.startOperationCall(operation);
                    }

                    PlasmaComponents3.ToolTip {
                        text: parent.Accessible.name
                    }
                }
            }
        }
    }

    header: PlasmaExtras.PlasmoidHeading {
        id: headerItem
        location: PlasmaExtras.PlasmoidHeading.Location.Header
        visible: playerList.model.length > 2 // more than one player, @multiplex is always there
        //this removes top padding to allow tabbar to touch the edge
        topPadding: topInset
        bottomPadding: -bottomInset
        implicitHeight: PlasmaCore.Units.gridUnit * 2
        PlasmaComponents3.TabBar {
            id: playerSelector
            position: PlasmaComponents3.TabBar.Header

            anchors.fill: parent

            implicitHeight: contentHeight

            Repeater {
                id: playerList
                model: root.mprisSourcesModel

                delegate: PlasmaComponents3.TabButton {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    icon.name: modelData["icon"]
                    icon.height: PlasmaCore.Units.iconSizes.smallMedium
                    Accessible.name: modelData["text"]
                    PlasmaComponents3.ToolTip {
                        text: modelData["text"]
                    }
                    // Keep the delegate centered by offsetting the padding removed in the parent
                    bottomPadding: verticalPadding + headerItem.bottomPadding
                    topPadding: verticalPadding - headerItem.bottomPadding
                    onClicked: {
                        disablePositionUpdate = true
                        mpris2Source.current = modelData["source"];
                        disablePositionUpdate = false
                    }
                }

                onModelChanged: {
                    playerSelector.currentIndex = model.findIndex(
                        (data) => { return data.source === mpris2Source.current }
                    )
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
