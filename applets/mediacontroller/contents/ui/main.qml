/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2020 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: root

    property var currentMetadata: mpris2Source.currentData ? mpris2Source.currentData.Metadata : undefined
    property string track: {
        if (!currentMetadata) {
            return ""
        }
        var xesamTitle = currentMetadata["xesam:title"]
        if (xesamTitle) {
            return xesamTitle
        }
        // if no track title is given, print out the file name
        var xesamUrl = currentMetadata["xesam:url"] ? currentMetadata["xesam:url"].toString() : ""
        if (!xesamUrl) {
            return ""
        }
        var lastSlashPos = xesamUrl.lastIndexOf('/')
        if (lastSlashPos < 0) {
            return ""
        }
        var lastUrlPart = xesamUrl.substring(lastSlashPos + 1)
        return decodeURIComponent(lastUrlPart)
    }
    property string artist: {
        if (!currentMetadata) {
            return ""
        }
        var xesamArtist = currentMetadata["xesam:artist"]
        if (!xesamArtist) {
            return "";
        }

        if (typeof xesamArtist == "string") {
            return xesamArtist
        } else {
            return xesamArtist.join(", ")
        }
    }
    property string albumArt: currentMetadata ? currentMetadata["mpris:artUrl"] || "" : ""

    readonly property string identity: !root.noPlayer ? mpris2Source.currentData.Identity || mpris2Source.current : ""

    property bool noPlayer: mpris2Source.sources.length <= 1

    property var mprisSourcesModel: []

    readonly property bool canControl: (!root.noPlayer && mpris2Source.currentData.CanControl) || false
    readonly property bool canGoPrevious: (canControl && mpris2Source.currentData.CanGoPrevious) || false
    readonly property bool canGoNext: (canControl && mpris2Source.currentData.CanGoNext) || false
    readonly property bool canPlay: (canControl && mpris2Source.currentData.CanPlay) || false
    readonly property bool canPause: (canControl && mpris2Source.currentData.CanPause) || false
    readonly property bool isPlaying: root.state === "playing"

    // var instead of bool so we can use "undefined" for "shuffle not supported"
    readonly property var shuffle: !root.noPlayer && typeof mpris2Source.currentData.Shuffle === "boolean"
                                   ? mpris2Source.currentData.Shuffle : undefined
    readonly property var loopStatus: !root.noPlayer && typeof mpris2Source.currentData.LoopStatus === "string"
                                      ? mpris2Source.currentData.LoopStatus : undefined

    readonly property int volumePercentStep: Plasmoid.configuration.volumeStep

    Plasmoid.switchWidth: PlasmaCore.Units.gridUnit * 14
    Plasmoid.switchHeight: PlasmaCore.Units.gridUnit * 10
    Plasmoid.icon: "media-playback-playing"
    Plasmoid.toolTipMainText: i18n("No media playing")
    Plasmoid.toolTipSubText: identity
    Plasmoid.toolTipTextFormat: Text.PlainText
    Plasmoid.status: PlasmaCore.Types.PassiveStatus

    Plasmoid.onContextualActionsAboutToShow: {
        Plasmoid.clearActions()

        if (root.noPlayer) {
            return
        }

        if (mpris2Source.currentData.CanRaise) {
            var icon = mpris2Source.currentData["Desktop Icon Name"] || ""
            Plasmoid.setAction("open", i18nc("Open player window or bring it to the front if already open", "Open"), icon)
        }

        if (canControl) {
            Plasmoid.setAction("previous", i18nc("Play previous track", "Previous Track"),
                               Qt.application.layoutDirection === Qt.RightToLeft ? "media-skip-forward" : "media-skip-backward");
            Plasmoid.action("previous").enabled = Qt.binding(function() {
                return root.canGoPrevious
            })

            // if CanPause, toggle the menu entry between Play & Pause, otherwise always use Play
            if (root.isPlaying && root.canPause) {
                Plasmoid.setAction("pause", i18nc("Pause playback", "Pause"), "media-playback-pause")
                Plasmoid.action("pause").enabled = Qt.binding(function() {
                    return root.isPlaying && root.canPause;
                });
            } else {
                Plasmoid.setAction("play", i18nc("Start playback", "Play"), "media-playback-start")
                Plasmoid.action("play").enabled = Qt.binding(function() {
                    return !root.isPlaying && root.canPlay;
                });
            }

            Plasmoid.setAction("next", i18nc("Play next track", "Next Track"),
                               Qt.application.layoutDirection === Qt.RightToLeft ? "media-skip-backward" : "media-skip-forward")
            Plasmoid.action("next").enabled = Qt.binding(function() {
                return root.canGoNext
            })

            Plasmoid.setAction("stop", i18nc("Stop playback", "Stop"), "media-playback-stop")
            Plasmoid.action("stop").enabled = Qt.binding(function() {
                return root.isPlaying || root.state === "paused";
            })
        }

        if (mpris2Source.currentData.CanQuit) {
            Plasmoid.setActionSeparator("quitseparator");
            Plasmoid.setAction("quit", i18nc("Quit player", "Quit"), "application-exit")
        }
    }

    // HACK Some players like Amarok take quite a while to load the next track
    // this avoids having the plasmoid jump between popup and panel
    onStateChanged: {
        if (state != "") {
            Plasmoid.status = PlasmaCore.Types.ActiveStatus
        } else {
            updatePlasmoidStatusTimer.restart()
        }
    }

    Timer {
        id: updatePlasmoidStatusTimer
        interval: 3000
        onTriggered: {
            if (state != "") {
                Plasmoid.status = PlasmaCore.Types.ActiveStatus
            } else {
                Plasmoid.status = PlasmaCore.Types.PassiveStatus
            }
        }
    }

    Plasmoid.compactRepresentation: CompactRepresentation {}
    Plasmoid.fullRepresentation: ExpandedRepresentation {}

    PlasmaCore.DataSource {
        id: mpris2Source

        readonly property string multiplexSource: "@multiplex"
        property string current: multiplexSource

        readonly property var currentData: data[current]

        engine: "mpris2"
        connectedSources: sources

        onSourceAdded: {
            updateMprisSourcesModel()
        }

        onSourceRemoved: {
            // if player is closed, reset to multiplex source
            if (source === current) {
                current = multiplexSource
            }
            updateMprisSourcesModel()
        }
    }

    Component.onCompleted: {
        mpris2Source.serviceForSource("@multiplex").enableGlobalShortcuts()
        updateMprisSourcesModel()
    }

    function togglePlaying() {
        if (root.isPlaying) {
            if (root.canPause) {
                root.action_pause();
            }
        } else {
            if (root.canPlay) {
                root.action_play();
            }
        }
    }

    function action_open() {
        serviceOp(mpris2Source.current, "Raise");
    }
    function action_quit() {
        serviceOp(mpris2Source.current, "Quit");
    }

    function action_play() {
        serviceOp(mpris2Source.current, "Play");
    }

    function action_pause() {
        serviceOp(mpris2Source.current, "Pause");
    }

    function action_playPause() {
        serviceOp(mpris2Source.current, "PlayPause");
    }

    function action_previous() {
        serviceOp(mpris2Source.current, "Previous");
    }

    function action_next() {
        serviceOp(mpris2Source.current, "Next");
    }

    function action_stop() {
        serviceOp(mpris2Source.current, "Stop");
    }

    function serviceOp(src, op) {
        var service = mpris2Source.serviceForSource(src);
        var operation = service.operationDescription(op);
        return service.startOperationCall(operation);
    }

    function updateMprisSourcesModel () {

        var model = [{
            'text': i18n("Choose player automatically"),
            'icon': 'emblem-favorite',
            'source': mpris2Source.multiplexSource
        }]

        var sources = mpris2Source.sources
        for (var i = 0, length = sources.length; i < length; ++i) {
            var source = sources[i]
            if (source === mpris2Source.multiplexSource) {
                continue
            }

            const playerData = mpris2Source.data[source];
            // source data is removed before its name is removed from the list
            if (!playerData) {
                continue;
            }

            model.push({
                'text': playerData["Identity"],
                'icon': playerData["Desktop Icon Name"] || playerData["DesktopEntry"] || "emblem-music-symbolic",
                'source': source
            });
        }

        root.mprisSourcesModel = model;
    }

    states: [
        State {
            name: "playing"
            when: !root.noPlayer && mpris2Source.currentData.PlaybackStatus === "Playing"

            PropertyChanges {
                target: Plasmoid.self
                icon: "media-playback-playing"
                toolTipMainText: track
                toolTipSubText: artist ? i18nc("by Artist (player name)", "by %1 (%2)", artist, identity) : identity
            }
        },
        State {
            name: "paused"
            when: !root.noPlayer && mpris2Source.currentData.PlaybackStatus === "Paused"

            PropertyChanges {
                target: Plasmoid.self
                icon: "media-playback-paused"
                toolTipMainText: track
                toolTipSubText: artist ? i18nc("by Artist (paused, player name)", "by %1 (paused, %2)", artist, identity) : i18nc("Paused (player name)", "Paused (%1)", identity)
            }
        }
    ]
}
