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
import org.kde.plasma.private.mediacontroller 1.0

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
        if (!xesamArtist || xesamArtist.length === 0) {
            xesamArtist = currentMetadata["xesam:albumArtist"] || [""]
        }
        return xesamArtist.join(", ")
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
    readonly property bool canRaise: (!root.noPlayer && mpris2Source.currentData.CanRaise) || false
    readonly property bool canQuit: (!root.noPlayer && mpris2Source.currentData.CanQuit) || false

    // var instead of bool so we can use "undefined" for "shuffle not supported"
    readonly property var shuffle: !root.noPlayer && typeof mpris2Source.currentData.Shuffle === "boolean"
                                   ? mpris2Source.currentData.Shuffle : undefined
    readonly property var loopStatus: !root.noPlayer && typeof mpris2Source.currentData.LoopStatus === "string"
                                      ? mpris2Source.currentData.LoopStatus : undefined

    GlobalConfig {
        id: config
    }

    readonly property int volumePercentStep: config.volumeStep

    Plasmoid.switchWidth: PlasmaCore.Units.gridUnit * 14
    Plasmoid.switchHeight: PlasmaCore.Units.gridUnit * 10
    Plasmoid.icon: "media-playback-playing"
    Plasmoid.toolTipMainText: i18n("No media playing")
    Plasmoid.toolTipSubText: identity
    Plasmoid.toolTipTextFormat: Text.PlainText
    Plasmoid.status: PlasmaCore.Types.PassiveStatus

    function populateContextualActions() {
        Plasmoid.clearActions()

        Plasmoid.setAction("open", i18nc("Open player window or bring it to the front if already open", "Open"),  "go-up-symbolic")
        Plasmoid.action("open").visible = Qt.binding(() => root.canRaise)
        Plasmoid.action("open").priority = Plasmoid.LowPriorityAction

        Plasmoid.setAction("previous", i18nc("Play previous track", "Previous Track"),
                           Qt.application.layoutDirection === Qt.RightToLeft ? "media-skip-forward" : "media-skip-backward");
        Plasmoid.action("previous").enabled = Qt.binding(() => root.canGoPrevious)
        Plasmoid.action("previous").visible = Qt.binding(() => root.canControl)
        Plasmoid.action("previous").priority = Plasmoid.LowPriorityAction

        Plasmoid.setAction("pause", i18nc("Pause playback", "Pause"), "media-playback-pause")
        Plasmoid.action("pause").enabled = Qt.binding(() => root.state === "playing" && root.canPause)
        Plasmoid.action("pause").visible = Qt.binding(() => root.canControl && root.state === "playing" && root.canPause)
        Plasmoid.action("pause").priority = Plasmoid.LowPriorityAction

        Plasmoid.setAction("play", i18nc("Start playback", "Play"), "media-playback-start")
        Plasmoid.action("play").enabled = Qt.binding(() => root.state !== "playing" && root.canPlay)
        Plasmoid.action("play").visible = Qt.binding(() => root.canControl && root.state !== "playing")
        Plasmoid.action("play").priority = Plasmoid.LowPriorityAction

        Plasmoid.setAction("next", i18nc("Play next track", "Next Track"),
                               Qt.application.layoutDirection === Qt.RightToLeft ? "media-skip-backward" : "media-skip-forward")
        Plasmoid.action("next").enabled = Qt.binding(() => root.canGoNext)
        Plasmoid.action("next").visible = Qt.binding(() => root.canControl)
        Plasmoid.action("next").priority = Plasmoid.LowPriorityAction

        Plasmoid.setAction("stop", i18nc("Stop playback", "Stop"), "media-playback-stop")
        Plasmoid.action("stop").enabled = Qt.binding(() => root.state === "playing" || root.state === "paused")
        Plasmoid.action("stop").visible = Qt.binding(() => root.canControl)
        Plasmoid.action("stop").priority = Plasmoid.LowPriorityAction

        Plasmoid.setActionSeparator("quitseparator");
        Plasmoid.action("quitseparator").visible = Qt.binding(() => root.canQuit)
        Plasmoid.action("quitseparator").priority = Plasmoid.LowPriorityAction

        Plasmoid.setAction("quit", i18nc("Quit player", "Quit"), "application-exit")
        Plasmoid.action("quit").visible = Qt.binding(() => root.canQuit)
        Plasmoid.action("quit").priority = Plasmoid.LowPriorityAction
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
        plasmoid.removeAction("configure");
        mpris2Source.serviceForSource("@multiplex").enableGlobalShortcuts()
        updateMprisSourcesModel()
        populateContextualActions()
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

        var proxyPIDs = [];  // for things like plasma-browser-integration
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


            if ("kde:pid" in playerData["Metadata"]) {
                var proxyPID = playerData["Metadata"]["kde:pid"];
                if (!proxyPIDs.includes(proxyPID)) {
                    proxyPIDs.push(proxyPID);
                }
            }
        }

        // prefer proxy controls like plasma-browser-integration over browser built-in controls
        model = model.filter( item => {
            if (mpris2Source.data[item["source"]] && "InstancePid" in mpris2Source.data[item["source"]]) {
                return !(proxyPIDs.includes(mpris2Source.data[item["source"]]["InstancePid"]));
            }
            return true;
        });

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
                toolTipSubText: artist ? i18nc("@info:tooltip %1 is a musical artist and %2 is an app name", "by %1 (%2)\nMiddle-click to pause\nScroll to adjust volume", artist, identity) : i18nc("@info:tooltip %1 is an app name", "%1\nMiddle-click to pause\nScroll to adjust volume", identity)
            }
        },
        State {
            name: "paused"
            when: !root.noPlayer && mpris2Source.currentData.PlaybackStatus === "Paused"

            PropertyChanges {
                target: Plasmoid.self
                icon: "media-playback-paused"
                toolTipMainText: track
                toolTipSubText: artist ? i18nc("@info:tooltip %1 is a musical artist and %2 is an app name", "by %1 (paused, %2)\nMiddle-click to play\nScroll to adjust volume", artist, identity) : i18nc("@info:tooltip %1 is an app name", "Paused (%1)\nMiddle-click to play\nScroll to adjust volume", identity)
            }
        }
    ]
}
