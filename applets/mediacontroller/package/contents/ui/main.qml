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
import org.kde.plasma.plasma5support 2.0 as P5Support
import org.kde.plasma.private.mediacontroller 1.0
import org.kde.kirigami 2.20 as Kirigami

PlasmoidItem {
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

    switchWidth: Kirigami.Units.gridUnit * 14
    switchHeight: Kirigami.Units.gridUnit * 10
    Plasmoid.icon: "media-playback-playing"
    toolTipMainText: i18n("No media playing")
    toolTipSubText: identity
    toolTipTextFormat: Text.PlainText
    Plasmoid.status: PlasmaCore.Types.PassiveStatus

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: i18nc("Open player window or bring it to the front if already open", "Open")
            icon.name: "go-up-symbolic"
            priority: PlasmaCore.Action.LowPriority
            visible: root.canRaise
            onTriggered: raise()
        },
        PlasmaCore.Action {
            text: i18nc("Play previous track", "Previous Track")
            icon.name: Qt.application.layoutDirection === Qt.RightToLeft ? "media-skip-forward" : "media-skip-backward"
            priority: PlasmaCore.Action.LowPriority
            visible: root.canControl
            enabled: root.canGoPrevious
            onTriggered: previous()
        },
        PlasmaCore.Action {
            text: i18nc("Pause playback", "Pause")
            icon.name: "media-playback-pause"
            priority: PlasmaCore.Action.LowPriority
            visible: root.canControl && root.state === "playing" && root.canPause
            enabled: root.canControl && root.state === "playing" && root.canPause
            onTriggered: pause()
        },
        PlasmaCore.Action {
            text: i18nc("Start playback", "Play")
            icon.name: "media-playback-start"
            priority: PlasmaCore.Action.LowPriority
            visible: root.canControl && root.state !== "playing"
            enabled: root.state !== "playing" && root.canPlay
            onTriggered: play()
        },
        PlasmaCore.Action {
            text: i18nc("Play next track", "Next Track")
            icon.name: Qt.application.layoutDirection === Qt.RightToLeft ? "media-skip-backward" : "media-skip-forward"
            priority: PlasmaCore.Action.LowPriority
            visible: root.canControl
            enabled: root.canGoNext
            onTriggered: next()
        },
        PlasmaCore.Action {
            text: i18nc("Stop playback", "Stop")
            icon.name: "media-playback-stop"
            priority: PlasmaCore.Action.LowPriority
            visible: root.canControl
            enabled: root.state === "playing" || root.state === "paused"
            onTriggered: stop()
        },
        PlasmaCore.Action {
            isSeparator: true
            priority: PlasmaCore.Action.LowPriority
            visible: root.canQuit
        },
        PlasmaCore.Action {
            text: i18nc("Quit player", "Quit")
            icon.name: "application-exit"
            priority: PlasmaCore.Action.LowPriority
            visible: root.canQuit
            onTriggered: quit()
        }
    ]
    function populateContextualActions() {

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

    compactRepresentation: CompactRepresentation {}
    fullRepresentation: ExpandedRepresentation {}

    P5Support.DataSource {
        id: mpris2Source

        readonly property string multiplexSource: "@multiplex"
        property string current: multiplexSource

        readonly property var currentData: data[current]

        engine: "mpris2"
        connectedSources: sources

        onSourceAdded: source => {
            updateMprisSourcesModel()
        }

        onSourceRemoved: source => {
            // if player is closed, reset to multiplex source
            if (source === current) {
                current = multiplexSource
            }
            updateMprisSourcesModel()
        }
    }

    Component.onCompleted: {
        Plasmoid.removeInternalAction("configure");
        mpris2Source.serviceForSource("@multiplex").enableGlobalShortcuts()
        updateMprisSourcesModel()
    }

    function previous() {
        __serviceOp(mpris2Source.current, "Previous");
    }
    function next() {
        __serviceOp(mpris2Source.current, "Next");
    }
    function play() {
        __serviceOp(mpris2Source.current, "Play");
    }
    function pause() {
        __serviceOp(mpris2Source.current, "Pause");
    }
    function togglePlaying() {
        if (root.isPlaying) {
            if (root.canPause) {
                pause();
            }
        } else {
            if (root.canPlay) {
                play();
            }
        }
    }
    function stop() {
        __serviceOp(mpris2Source.current, "Stop");
    }
    function quit() {
        __serviceOp(mpris2Source.current, "Quit");
    }
    function raise() {
        __serviceOp(mpris2Source.current, "Raise");
    }

    function __serviceOp(src, op) {
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
                target: Plasmoid
                icon: "media-playback-playing"
            }
            PropertyChanges {
                target: root
                toolTipMainText: track
                toolTipSubText: artist ? i18nc("@info:tooltip %1 is a musical artist and %2 is an app name", "by %1 (%2)\nMiddle-click to pause\nScroll to adjust volume", artist, identity) : i18nc("@info:tooltip %1 is an app name", "%1\nMiddle-click to pause\nScroll to adjust volume", identity)
            }
        },
        State {
            name: "paused"
            when: !root.noPlayer && mpris2Source.currentData.PlaybackStatus === "Paused"

            PropertyChanges {
                target: Plasmoid
                icon: "media-playback-paused"
            }
            PropertyChanges {
                target: root
                toolTipMainText: track
                toolTipSubText: artist ? i18nc("@info:tooltip %1 is a musical artist and %2 is an app name", "by %1 (paused, %2)\nMiddle-click to play\nScroll to adjust volume", artist, identity) : i18nc("@info:tooltip %1 is an app name", "Paused (%1)\nMiddle-click to play\nScroll to adjust volume", identity)
            }
        }
    ]
}
