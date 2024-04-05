/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014, 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
    SPDX-FileCopyrightText: 2020 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.coreaddons 1.0 as KCoreAddons
import org.kde.kirigami 2 as Kirigami
import org.kde.plasma.private.mpris as Mpris

PlasmaExtras.Representation {
    id: expandedRepresentation

    Layout.minimumWidth: switchWidth
    Layout.minimumHeight: switchHeight
    Layout.preferredWidth: Kirigami.Units.gridUnit * 20
    Layout.preferredHeight: Kirigami.Units.gridUnit * 20
    Layout.maximumWidth: Kirigami.Units.gridUnit * 40
    Layout.maximumHeight: Kirigami.Units.gridUnit * 40

    collapseMarginsHint: true

    readonly property alias playerSelector: playerSelector
    readonly property int controlSize: Kirigami.Units.iconSizes.medium

    readonly property bool softwareRendering: GraphicsInfo.api === GraphicsInfo.Software
    readonly property var appletInterface: root
    property real rate: mpris2Model.currentPlayer?.rate ?? 1
    property double length: mpris2Model.currentPlayer?.length ?? 0
    property double position: mpris2Model.currentPlayer?.position ?? 0
    property bool canSeek: mpris2Model.currentPlayer?.canSeek ?? false

    // only show hours (the default for KFormat) when track is actually longer than an hour
    readonly property int durationFormattingOptions: length >= 60*60*1000*1000 ? 0 : KCoreAddons.FormatTypes.FoldHours

    property bool disablePositionUpdate: false
    property bool keyPressed: false

    KeyNavigation.tab: playerSelector.count ? playerSelector.currentItem : (seekSlider.visible ? seekSlider : seekSlider.KeyNavigation.down)
    KeyNavigation.down: KeyNavigation.tab

    onPositionChanged: {
        // we don't want to interrupt the user dragging the slider
        if (!seekSlider.pressed && !keyPressed) {
            // we also don't want passive position updates
            disablePositionUpdate = true
            // Slider refuses to set value beyond its end, make sure "to" is up-to-date first
            seekSlider.to = length;
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
        mpris2Model.currentPlayer?.updatePosition();
        disablePositionUpdate = false
    }

    Keys.onPressed: keyPressed = true

    Keys.onReleased: event => {
        keyPressed = false

        if ((event.key == Qt.Key_Tab || event.key == Qt.Key_Backtab) && event.modifiers & Qt.ControlModifier) {
            event.accepted = true;
            if (playerList.count > 2) {
                let nextIndex = mpris2Model.currentIndex + 1;
                if (event.key == Qt.Key_Backtab || event.modifiers & Qt.ShiftModifier) {
                    nextIndex -= 2;
                }
                if (nextIndex == playerList.count) {
                    nextIndex = 0;
                }
                if (nextIndex < 0) {
                    nextIndex = playerList.count - 1;
                }
                mpris2Model.currentIndex = nextIndex;
            }
        }

        if (!event.modifiers) {
            event.accepted = true

            if (event.key === Qt.Key_Space || event.key === Qt.Key_K) {
                // K is YouTube's key for "play/pause" :)
                root.togglePlaying()
            } else if (event.key === Qt.Key_P) {
                root.previous()
            } else if (event.key === Qt.Key_N) {
                root.next()
            } else if (event.key === Qt.Key_S) {
                root.stop()
            } else if (event.key === Qt.Key_J) { // TODO ltr languages
                // seek back 5s
                seekSlider.value = Math.max(0, seekSlider.value - 5000000) // microseconds
                seekSlider.moved();
            } else if (event.key === Qt.Key_L) {
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

    Timer {
        id: queuedPositionUpdate
        interval: 100
        onTriggered: {
            if (expandedRepresentation.position == seekSlider.value) {
                return;
            }
            mpris2Model.currentPlayer.position = seekSlider.value;
        }
    }

    // Album Art Background + Details + Touch area to adjust position or volume
    MultiPointTouchArea {
        id: touchArea
        anchors.fill: parent
        clip: true

        maximumTouchPoints: 1
        minimumTouchPoints: 1
        mouseEnabled: false
        touchPoints: [
            TouchPoint {
                id: point1

                property bool seeking: false
                property bool adjustingVolume: false

                onPressedChanged: if (!pressed) {
                    seeking = false;
                    adjustingVolume = false;
                }
                onSeekingChanged: if (seeking) {
                    queuedPositionUpdate.stop();
                } else {
                    seekSlider.moved();
                }
            }
        ]

        Connections {
            enabled: seekSlider.visible && point1.pressed && !point1.adjustingVolume
            target: point1
            // Control seek slider
            function onXChanged() {
                if (!point1.seeking && Math.abs(point1.x - point1.startX) < touchArea.width / 20) {
                    return;
                }
                point1.seeking = true;
                seekSlider.value = seekSlider.valueAt(Math.max(0, Math.min(1, seekSlider.position + (point1.x - point1.previousX) / touchArea.width))); // microseconds
            }
        }

        Connections {
            enabled: point1.pressed && !point1.seeking
            target: point1
            function onYChanged() {
                if (!point1.adjustingVolume && Math.abs(point1.y - point1.startY) < touchArea.height / 20) {
                    return;
                }
                point1.adjustingVolume = true;
                mpris2Model.currentPlayer.changeVolume((point1.previousY - point1.y) / touchArea.height, false);
            }
        }

        ShaderEffect {
            id: backgroundImage
            property real scaleFactor: 1.0
            property ShaderEffectSource source: ShaderEffectSource {
                id: shaderEffectSource
                sourceItem: albumArt.albumArt
            }

            anchors.centerIn: parent
            visible: (albumArt.animating || albumArt.hasImage) && !softwareRendering

            layer.enabled: !softwareRendering
            layer.effect: HueSaturation {
                cached: true

                lightness: -0.5
                saturation: 0.9

                layer.enabled: true
                layer.effect: FastBlur {
                    cached: true

                    radius: 128

                    transparentBorder: false
                }
            }
            // use State to avoid unnecessary reevaluation of width and height
            states: State {
                name: "albumArtReady"
                when: root.expanded && backgroundImage.visible && shaderEffectSource.sourceItem.currentItem?.paintedWidth > 0
                PropertyChanges {
                    target: backgroundImage
                    scaleFactor: Math.max(parent.width / shaderEffectSource.sourceItem.currentItem.paintedWidth, parent.height / shaderEffectSource.sourceItem.currentItem.paintedHeight)
                    width: Math.round(shaderEffectSource.sourceItem.currentItem.paintedWidth * scaleFactor)
                    height: Math.round(shaderEffectSource.sourceItem.currentItem.paintedHeight * scaleFactor)
                }
                PropertyChanges {
                    target: shaderEffectSource
                    // HACK: Fix background ratio when DPI > 1
                    sourceRect: Qt.rect(shaderEffectSource.sourceItem.width - shaderEffectSource.sourceItem.currentItem.paintedWidth,
                                    Math.round((shaderEffectSource.sourceItem.height - shaderEffectSource.sourceItem.currentItem.paintedHeight) / 2),
                                    shaderEffectSource.sourceItem.currentItem.paintedWidth,
                                    shaderEffectSource.sourceItem.currentItem.paintedHeight)
                }
            }
        }
        Item { // Album Art + Details
            id: albumRow

            anchors {
                fill: parent
                leftMargin: Kirigami.Units.gridUnit
                rightMargin: Kirigami.Units.gridUnit
            }

            AlbumArtStackView {
                id: albumArt

                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.left
                    right: detailsColumn.visible ? parent.horizontalCenter : parent.right
                    rightMargin: Kirigami.Units.gridUnit / 2
                }

                Connections {
                    enabled: root.expanded
                    target: root

                    function onAlbumArtChanged() {
                        albumArt.loadAlbumArt();
                    }
                }

                Connections {
                    target: root

                    function onExpandedChanged() {
                        if (!root.expanded) {
                            return;
                        } else if (albumArt.albumArt.currentItem instanceof Image && albumArt.albumArt.currentItem.source.toString() === Qt.resolvedUrl(root.albumArt).toString()) {
                            // QTBUG-119904 StackView ignores transitions when it's invisible
                            albumArt.albumArt.currentItem.opacity = 1;
                        } else {
                            albumArt.loadAlbumArt();
                        }
                    }
                }
            }

            ColumnLayout { // Details Column
                id: detailsColumn
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.horizontalCenter
                    leftMargin: Kirigami.Units.gridUnit / 2
                    right: parent.right
                }
                visible: root.track.length > 0

                Item {
                    Layout.fillHeight: true
                }

                Kirigami.Heading { // Song Title
                    id: songTitle
                    level: 1

                    color: (softwareRendering || !albumArt.hasImage) ? Kirigami.Theme.textColor : "white"

                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.VerticalFit
                    elide: Text.ElideRight

                    text: root.track

                    Layout.fillWidth: true
                    Layout.maximumHeight: Kirigami.Units.gridUnit * 5
                }
                Kirigami.Heading { // Song Artist
                    id: songArtist
                    visible: root.artist
                    level: 2

                    color: (softwareRendering || !albumArt.hasImage) ? Kirigami.Theme.textColor : "white"

                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.VerticalFit
                    elide: Text.ElideRight

                    text: root.artist
                    Layout.fillWidth: true
                    Layout.maximumHeight: Kirigami.Units.gridUnit * 2
                }
                Kirigami.Heading { // Song Album
                    color: (softwareRendering || !albumArt.hasImage) ? Kirigami.Theme.textColor : "white"

                    level: 3
                    opacity: 0.6

                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.VerticalFit
                    elide: Text.ElideRight

                    visible: text.length > 0
                    text: root.album
                    Layout.fillWidth: true
                    Layout.maximumHeight: Kirigami.Units.gridUnit * 2
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    footer: PlasmaExtras.PlasmoidHeading {
        id: footerItem
        position: PlasmaComponents3.ToolBar.Footer
        ColumnLayout { // Main Column Layout
            anchors.fill: parent
            RowLayout { // Seek Bar
                spacing: Kirigami.Units.smallSpacing

                // if there's no "mpris:length" in the metadata, we cannot seek, so hide it in that case
                enabled: playerList.count > 0 && root.track.length > 0 && expandedRepresentation.length > 0 ? true : false
                opacity: enabled ? 1 : 0
                Behavior on opacity {
                    NumberAnimation { duration: Kirigami.Units.longDuration }
                }

                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.maximumWidth: Math.min(Kirigami.Units.gridUnit * 45, Math.round(expandedRepresentation.width * (7 / 10)))

                // ensure the layout doesn't shift as the numbers change and measure roughly the longest text that could occur with the current song
                TextMetrics {
                    id: timeMetrics
                    text: i18nc("Remaining time for song e.g -5:42", "-%1",
                                KCoreAddons.Format.formatDuration(seekSlider.to / 1000, expandedRepresentation.durationFormattingOptions))
                    font: Kirigami.Theme.smallFont
                }

                PlasmaComponents3.Label { // Time Elapsed
                    Layout.preferredWidth: timeMetrics.width
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    text: KCoreAddons.Format.formatDuration(seekSlider.value / 1000, expandedRepresentation.durationFormattingOptions)
                    opacity: 0.9
                    font: Kirigami.Theme.smallFont
                    color: Kirigami.Theme.textColor
                    textFormat: Text.PlainText
                }

                PlasmaComponents3.Slider { // Slider
                    id: seekSlider
                    Layout.fillWidth: true
                    z: 999
                    value: 0
                    visible: canSeek

                    KeyNavigation.backtab: playerSelector.currentItem
                    KeyNavigation.up: KeyNavigation.backtab
                    KeyNavigation.down: playPauseButton.enabled ? playPauseButton : (playPauseButton.KeyNavigation.left.enabled ? playPauseButton.KeyNavigation.left : playPauseButton.KeyNavigation.right)
                    Keys.onLeftPressed: {
                        seekSlider.value = Math.max(0, seekSlider.value - 5000000) // microseconds
                        seekSlider.moved();
                    }
                    Keys.onRightPressed: {
                        seekSlider.value = Math.max(0, seekSlider.value + 5000000) // microseconds
                        seekSlider.moved();
                    }

                    onMoved: {
                        if (!disablePositionUpdate) {
                            // delay setting the position to avoid race conditions
                            queuedPositionUpdate.restart()
                        }
                    }
                    onPressedChanged: {
                        // Property binding evaluation is non-deterministic
                        // so binding visible to pressed and delay to 0 when pressed
                        // will not make the tooltip show up immediately.
                        if (pressed) {
                            seekToolTip.delay = 0;
                            seekToolTip.visible = true;
                        } else {
                            seekToolTip.delay = Qt.binding(() => Kirigami.Units.toolTipDelay);
                            seekToolTip.visible = Qt.binding(() => seekToolTipHandler.hovered);
                        }
                    }

                    HoverHandler {
                        id: seekToolTipHandler
                    }

                    PlasmaComponents3.ToolTip {
                        id: seekToolTip
                        readonly property real position: {
                            if (seekSlider.pressed) {
                                return seekSlider.visualPosition;
                            }
                            // does not need mirroring since we work on raw mouse coordinates
                            const mousePos = seekToolTipHandler.point.position.x - seekSlider.handle.width / 2;
                            return Math.max(0, Math.min(1, mousePos / (seekSlider.width - seekSlider.handle.width)));
                        }
                        x: Math.round(seekSlider.handle.width / 2 + position * (seekSlider.width - seekSlider.handle.width) - width / 2)
                        // Never hide (not on press, no timeout) as long as the mouse is hovered
                        closePolicy: PlasmaComponents3.Popup.NoAutoClose
                        timeout: -1
                        text: {
                            // Label text needs mirrored position again
                            const effectivePosition = seekSlider.mirrored ? (1 - position) : position;
                            return KCoreAddons.Format.formatDuration((seekSlider.to - seekSlider.from) * effectivePosition / 1000, expandedRepresentation.durationFormattingOptions)
                        }
                        // NOTE also controlled in onPressedChanged handler above
                        visible: seekToolTipHandler.hovered
                    }

                    Timer {
                        id: seekTimer
                        interval: 1000 / expandedRepresentation.rate
                        repeat: true
                        running: root.isPlaying && root.expanded && !keyPressed && interval > 0 && seekSlider.to >= 1000000
                        onTriggered: {
                            // some players don't continuously update the seek slider position via mpris
                            // add one second; value in microseconds
                            if (!seekSlider.pressed) {
                                disablePositionUpdate = true
                                if (seekSlider.value == seekSlider.to) {
                                    mpris2Model.currentPlayer.updatePosition();
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
                    font: Kirigami.Theme.smallFont
                    color: Kirigami.Theme.textColor
                    textFormat: Text.PlainText
                }
            }

            RowLayout { // Player Controls
                id: playerControls

                property int controlsSize: Kirigami.Units.gridUnit * 3

                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.ToolButton {
                    id: shuffleButton
                    Layout.rightMargin: LayoutMirroring.enabled ? 0 : Kirigami.Units.gridUnit - playerControls.spacing
                    Layout.leftMargin: LayoutMirroring.enabled ? Kirigami.Units.gridUnit - playerControls.spacing : 0
                    icon.name: "media-playlist-shuffle"
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize
                    checked: root.shuffle === Mpris.ShuffleStatus.On
                    enabled: root.canControl && root.shuffle !== Mpris.ShuffleStatus.Unknown

                    display: PlasmaComponents3.AbstractButton.IconOnly
                    text: i18nc("@action:button", "Shuffle")

                    KeyNavigation.right: previousButton.enabled ? previousButton : previousButton.KeyNavigation.right
                    KeyNavigation.up: playPauseButton.KeyNavigation.up

                    onClicked: {
                        mpris2Model.currentPlayer.shuffle =
                            root.shuffle === Mpris.ShuffleStatus.On ? Mpris.ShuffleStatus.Off : Mpris.ShuffleStatus.On;
                    }

                    PlasmaComponents3.ToolTip {
                        text: parent.text
                    }
                }

                PlasmaComponents3.ToolButton { // Previous
                    id: previousButton
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize
                    Layout.alignment: Qt.AlignVCenter
                    enabled: root.canGoPrevious
                    icon.name: LayoutMirroring.enabled ? "media-skip-forward" : "media-skip-backward"

                    display: PlasmaComponents3.AbstractButton.IconOnly
                    text: i18nc("Play previous track", "Previous Track")

                    KeyNavigation.left: shuffleButton
                    KeyNavigation.right: playPauseButton.enabled ? playPauseButton : playPauseButton.KeyNavigation.right
                    KeyNavigation.up: playPauseButton.KeyNavigation.up

                    onClicked: {
                        seekSlider.value = 0    // Let the media start from beginning. Bug 362473
                        root.previous()
                    }
                }

                PlasmaComponents3.ToolButton { // Pause/Play
                    id: playPauseButton
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize

                    Layout.alignment: Qt.AlignVCenter
                    enabled: root.isPlaying ? root.canPause : root.canPlay
                    icon.name: root.isPlaying ? "media-playback-pause" : "media-playback-start"

                    display: PlasmaComponents3.AbstractButton.IconOnly
                    text: root.isPlaying ? i18nc("Pause playback", "Pause") : i18nc("Start playback", "Play")

                    KeyNavigation.left: previousButton.enabled ? previousButton : previousButton.KeyNavigation.left
                    KeyNavigation.right: nextButton.enabled ? nextButton : nextButton.KeyNavigation.right
                    KeyNavigation.up: seekSlider.visible ? seekSlider : seekSlider.KeyNavigation.up

                    onClicked: root.togglePlaying()
                }

                PlasmaComponents3.ToolButton { // Next
                    id: nextButton
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize
                    Layout.alignment: Qt.AlignVCenter
                    enabled: root.canGoNext
                    icon.name: LayoutMirroring.enabled ? "media-skip-backward" : "media-skip-forward"

                    display: PlasmaComponents3.AbstractButton.IconOnly
                    text: i18nc("Play next track", "Next Track")

                    KeyNavigation.left: playPauseButton.enabled ? playPauseButton : playPauseButton.KeyNavigation.left
                    KeyNavigation.right: repeatButton
                    KeyNavigation.up: playPauseButton.KeyNavigation.up

                    onClicked: {
                        seekSlider.value = 0    // Let the media start from beginning. Bug 362473
                        root.next()
                    }
                }

                PlasmaComponents3.ToolButton {
                    id: repeatButton
                    Layout.leftMargin: LayoutMirroring.enabled ? 0 : Kirigami.Units.gridUnit - playerControls.spacing
                    Layout.rightMargin: LayoutMirroring.enabled ? Kirigami.Units.gridUnit - playerControls.spacing : 0
                    icon.name: root.loopStatus === Mpris.LoopStatus.Track ? "media-playlist-repeat-song" : "media-playlist-repeat"
                    icon.width: expandedRepresentation.controlSize
                    icon.height: expandedRepresentation.controlSize
                    checked: root.loopStatus !== Mpris.LoopStatus.Unknown && root.loopStatus !== Mpris.LoopStatus.None
                    enabled: root.canControl && root.loopStatus !== Mpris.LoopStatus.Unknown

                    display: PlasmaComponents3.AbstractButton.IconOnly
                    text: root.loopStatus === Mpris.LoopStatus.Track ? i18n("Repeat Track") : i18n("Repeat")

                    KeyNavigation.left: nextButton.enabled ? nextButton : nextButton.KeyNavigation.left
                    KeyNavigation.up: playPauseButton.KeyNavigation.up

                    onClicked: {
                        let status;
                        switch (root.loopStatus) {
                        case Mpris.LoopStatus.Playlist:
                            status = Mpris.LoopStatus.Track;
                            break;
                        case Mpris.LoopStatus.Track:
                            status = Mpris.LoopStatus.None;
                            break;
                        default:
                            status = Mpris.LoopStatus.Playlist;
                        }
                        mpris2Model.currentPlayer.loopStatus = status;
                    }

                    PlasmaComponents3.ToolTip {
                        text: parent.text
                    }
                }
            }
        }
    }

    header: PlasmaExtras.PlasmoidHeading {
        id: headerItem
        position: PlasmaComponents3.ToolBar.Header
        visible: playerSelector.count > (Plasmoid.configuration.multiplexerEnabled ? 2 : 1)
        //this removes top padding to allow tabbar to touch the edge
        topPadding: topInset
        bottomPadding: -bottomInset
        implicitHeight: Kirigami.Units.gridUnit * 2

        PlasmaComponents3.TabBar {
            id: playerSelector
            objectName: "playerSelector"

            anchors.fill: parent
            implicitHeight: contentHeight
            currentIndex: playerSelector.count, mpris2Model.currentIndex
            position: PlasmaComponents3.TabBar.Header

            Repeater {
                id: playerList
                model: mpris2Model
                delegate: PlasmaComponents3.TabButton {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    implicitWidth: 1 // HACK: suppress binding loop warnings
                    readonly property QtObject m: model
                    display: PlasmaComponents3.AbstractButton.IconOnly
                    icon.name: model.iconName
                    icon.height: Kirigami.Units.iconSizes.smallMedium
                    text: model.identity
                    // Keep the delegate centered by offsetting the padding removed in the parent
                    bottomPadding: verticalPadding + headerItem.bottomPadding
                    topPadding: verticalPadding - headerItem.bottomPadding

                    Accessible.onPressAction: clicked()
                    KeyNavigation.down: seekSlider.visible ? seekSlider : seekSlider.KeyNavigation.down

                    onClicked: {
                        mpris2Model.currentIndex = index;
                    }

                    PlasmaComponents3.ToolTip.text: text
                    PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
                    PlasmaComponents3.ToolTip.visible: hovered || (activeFocus && (focusReason === Qt.TabFocusReason || focusReason === Qt.BacktabFocusReason))
                }
            }
        }
    }
}
