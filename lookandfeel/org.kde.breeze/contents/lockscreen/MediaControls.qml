/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    visible: mpris2Source.hasPlayer
    implicitHeight: PlasmaCore.Units.gridUnit * 3
    implicitWidth: PlasmaCore.Units.gridUnit * 16

    RowLayout {
        id: controlsRow
        anchors.fill: parent
        spacing: 0

        enabled: mpris2Source.canControl

        PlasmaCore.DataSource {
            id: mpris2Source

            readonly property string source: "@multiplex"
            readonly property var playerData: data[source]

            readonly property bool hasPlayer: sources.length > 1 && !!playerData
            readonly property string identity: hasPlayer && playerData.Identity || ""
            readonly property bool playing: hasPlayer && playerData.PlaybackStatus === "Playing"
            readonly property bool canControl: hasPlayer && playerData.CanControl
            readonly property bool canGoBack: hasPlayer && playerData.CanGoPrevious
            readonly property bool canGoNext: hasPlayer && playerData.CanGoNext

            readonly property var currentMetadata: hasPlayer ? playerData.Metadata : ({})

            readonly property string track: {
                const xesamTitle = currentMetadata["xesam:title"]
                if (xesamTitle) {
                    return xesamTitle
                }
                // if no track title is given, print out the file name
                const xesamUrl = currentMetadata["xesam:url"] ? currentMetadata["xesam:url"].toString() : ""
                if (!xesamUrl) {
                    return ""
                }
                const lastSlashPos = xesamUrl.lastIndexOf('/')
                if (lastSlashPos < 0) {
                    return ""
                }
                const lastUrlPart = xesamUrl.substring(lastSlashPos + 1)
                return decodeURIComponent(lastUrlPart)
            }
            readonly property var artists: currentMetadata["xesam:artist"] || [] // stringlist
            readonly property var albumArtists: currentMetadata["xesam:albumArtist"] || [] // stringlist
            readonly property string albumArt: currentMetadata["mpris:artUrl"] || ""

            engine: "mpris2"
            connectedSources: [source]

            function startOperation(op) {
                const service = serviceForSource(source)
                const operation = service.operationDescription(op)
                return service.startOperationCall(operation)
            }

            function goPrevious() {
                startOperation("Previous");
            }
            function goNext() {
                startOperation("Next");
            }
            function playPause(source) {
                startOperation("PlayPause");
            }
        }

        Image {
            id: albumArt
            Layout.preferredWidth: height
            Layout.fillHeight: true
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            source: mpris2Source.albumArt
            sourceSize.height: height
            visible: status === Image.Loading || status === Image.Ready
        }

        Item { // spacer
            width: PlasmaCore.Units.smallSpacing
            height: 1
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            PlasmaComponents3.Label {
                Layout.fillWidth: true
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                text: mpris2Source.track.length > 0
                        ? mpris2Source.track
                        : ((mpris2Source.hasPlayer && ["Playing", "Paused"].includes(mpris2Source.playerData.PlaybackStatus))
                            ? i18nd("plasma_lookandfeel_org.kde.lookandfeel", "No title")
                            : i18nd("plasma_lookandfeel_org.kde.lookandfeel", "No media playing"))
                textFormat: Text.PlainText
                font.pointSize: PlasmaCore.Theme.defaultFont.pointSize + 1
                maximumLineCount: 1
            }

            PlasmaExtras.DescriptiveLabel {
                Layout.fillWidth: true
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                // if no artist is given, show player name instead
                text: mpris2Source.artists.length > 0 ? mpris2Source.artists.join(", ") : (mpris2Source.albumArtists.length > 0 ? mpris2Source.albumArtists.join(", ") : mpris2Source.identity)
                textFormat: Text.PlainText
                font.pointSize: PlasmaCore.Theme.smallestFont.pointSize + 1
                maximumLineCount: 1
            }
        }

        PlasmaComponents3.ToolButton {
            focusPolicy: Qt.TabFocus
            enabled: mpris2Source.canGoBack
            Layout.preferredHeight: PlasmaCore.Units.gridUnit*2
            Layout.preferredWidth: Layout.preferredHeight
            icon.name: LayoutMirroring.enabled ? "media-skip-forward" : "media-skip-backward"
            onClicked: {
                fadeoutTimer.running = false
                mpris2Source.goPrevious()
            }
            visible: mpris2Source.canGoBack || mpris2Source.canGoNext
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Previous track")
        }

        PlasmaComponents3.ToolButton {
            focusPolicy: Qt.TabFocus
            Layout.fillHeight: true
            Layout.preferredWidth: height // make this button bigger
            icon.name: mpris2Source.playing ? "media-playback-pause" : "media-playback-start"
            onClicked: {
                fadeoutTimer.running = false
                mpris2Source.playPause()
            }
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Play or Pause media")
        }

        PlasmaComponents3.ToolButton {
            focusPolicy: Qt.TabFocus
            enabled: mpris2Source.canGoNext
            Layout.preferredHeight: PlasmaCore.Units.gridUnit*2
            Layout.preferredWidth: Layout.preferredHeight
            icon.name: LayoutMirroring.enabled ? "media-skip-backward" : "media-skip-forward"
            onClicked: {
                fadeoutTimer.running = false
                mpris2Source.goNext()
            }
            visible: mpris2Source.canGoBack || mpris2Source.canGoNext
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Next track")
        }
    }
}
