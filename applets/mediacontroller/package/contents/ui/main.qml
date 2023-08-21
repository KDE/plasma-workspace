/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2020 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.private.mediacontroller 1.0
import org.kde.plasma.private.mpris as Mpris
import org.kde.kirigami 2 as Kirigami

PlasmoidItem {
    id: root

    switchWidth: Kirigami.Units.gridUnit * 14
    switchHeight: Kirigami.Units.gridUnit * 10

    readonly property int volumePercentStep: config.volumeStep

    // BEGIN model properties
    readonly property string track: mpris2Model.currentPlayer?.track ?? ""
    readonly property string artist: mpris2Model.currentPlayer?.artist ?? ""
    readonly property string album: mpris2Model.currentPlayer?.album ?? ""
    readonly property string albumArt: mpris2Model.currentPlayer?.artUrl ?? ""
    readonly property string identity: mpris2Model.currentPlayer?.identity ?? ""
    readonly property bool canControl: mpris2Model.currentPlayer?.canControl ?? false
    readonly property bool canGoPrevious: mpris2Model.currentPlayer?.canGoPrevious ?? false
    readonly property bool canGoNext: mpris2Model.currentPlayer?.canGoNext ?? false
    readonly property bool canPlay: mpris2Model.currentPlayer?.canPlay ?? false
    readonly property bool canPause: mpris2Model.currentPlayer?.canPause ?? false
    readonly property bool canStop: mpris2Model.currentPlayer?.canStop ?? false
    readonly property int playbackStatus: mpris2Model.currentPlayer?.playbackStatus ?? 0
    readonly property bool isPlaying: root.playbackStatus === Mpris.PlaybackStatus.Playing
    readonly property bool canRaise: mpris2Model.currentPlayer?.canRaise ?? false
    readonly property bool canQuit: mpris2Model.currentPlayer?.canQuit ?? false
    readonly property int shuffle: mpris2Model.currentPlayer?.shuffle ?? 0
    readonly property int loopStatus: mpris2Model.currentPlayer?.loopStatus ?? 0
    // END model properties

    Plasmoid.icon: switch (root.playbackStatus) {
    case Mpris.PlaybackStatus.Playing:
        return "media-playback-playing-symbolic";
    case Mpris.PlaybackStatus.Paused:
        return "media-playback-paused-symbolic";
    default:
        return "media-playback-stopped-symbolic";
    }
    Plasmoid.status: PlasmaCore.Types.PassiveStatus
    toolTipMainText: root.playbackStatus > Mpris.PlaybackStatus.Stopped ? root.track : i18n("No media playing")
    toolTipSubText: switch (root.playbackStatus) {
    case Mpris.PlaybackStatus.Playing:
        return root.artist ? i18nc("@info:tooltip %1 is a musical artist and %2 is an app name", "by %1 (%2)\nMiddle-click to pause\nScroll to adjust volume", root.artist, root.identity)
            : i18nc("@info:tooltip %1 is an app name", "%1\nMiddle-click to pause\nScroll to adjust volume", root.identity)
    case Mpris.PlaybackStatus.Paused:
        return root.artist ? i18nc("@info:tooltip %1 is a musical artist and %2 is an app name", "by %1 (paused, %2)\nMiddle-click to play\nScroll to adjust volume", root.artist, root.identity)
            : i18nc("@info:tooltip %1 is an app name", "Paused (%1)\nMiddle-click to play\nScroll to adjust volume", root.identity)
    default:
        return "";
    }
    toolTipTextFormat: Text.PlainText

    compactRepresentation: CompactRepresentation {}
    fullRepresentation: ExpandedRepresentation {}

    // HACK Some players like Amarok take quite a while to load the next track
    // this avoids having the plasmoid jump between popup and panel
    onPlaybackStatusChanged: {
        if (root.playbackStatus > Mpris.PlaybackStatus.Stopped) {
            Plasmoid.status = PlasmaCore.Types.ActiveStatus
        } else {
            updatePlasmoidStatusTimer.restart()
        }
    }

    onExpandedChanged: {
        if (root.expanded) {
            mpris2Model.currentPlayer?.updatePosition();
        }
    }

    Timer {
        id: updatePlasmoidStatusTimer
        interval: Kirigami.Units.humanMoment
        onTriggered: {
            if (root.playbackStatus > Mpris.PlaybackStatus.Stopped) {
                Plasmoid.status = PlasmaCore.Types.ActiveStatus
            } else {
                Plasmoid.status = PlasmaCore.Types.PassiveStatus
            }
        }
    }

    GlobalConfig {
        id: config
    }

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
            visible: root.isPlaying && root.canPause
            enabled: visible
            onTriggered: pause()
        },
        PlasmaCore.Action {
            text: i18nc("Start playback", "Play")
            icon.name: "media-playback-start"
            priority: PlasmaCore.Action.LowPriority
            visible: root.canControl && !root.isPlaying
            enabled: root.canPlay
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
            enabled: root.canStop
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

    function previous() {
        mpris2Model.currentPlayer.Previous();
    }
    function next() {
        mpris2Model.currentPlayer.Next();
    }
    function play() {
        mpris2Model.currentPlayer.Play();
    }
    function pause() {
        mpris2Model.currentPlayer.Pause();
    }
    function togglePlaying() {
        mpris2Model.currentPlayer.PlayPause();
    }
    function stop() {
        mpris2Model.currentPlayer.Stop();
    }
    function quit() {
        mpris2Model.currentPlayer.Quit();
    }
    function raise() {
        mpris2Model.currentPlayer.Raise();
    }

    Mpris.Mpris2Model {
        id: mpris2Model
    }

    Component.onCompleted: {
        Plasmoid.removeInternalAction("configure");
    }
}
