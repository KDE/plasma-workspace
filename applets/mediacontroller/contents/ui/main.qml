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
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
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
    property string artist: currentMetadata ? currentMetadata["xesam:artist"] || "" : ""
    property string albumArt: currentMetadata ? currentMetadata["mpris:artUrl"] || "" : ""

    property bool noPlayer: mpris2Source.sources.length <= 1

    readonly property bool canRaise: !root.noPlayer && mpris2Source.currentData.CanRaise
    readonly property bool canQuit: !root.noPlayer && mpris2Source.currentData.CanQuit

    readonly property bool canControl: !root.noPlayer && mpris2Source.currentData.CanControl
    readonly property bool canGoPrevious: canControl && mpris2Source.currentData.CanGoPrevious
    readonly property bool canGoNext: canControl && mpris2Source.currentData.CanGoNext

    Plasmoid.switchWidth: units.gridUnit * 14
    Plasmoid.switchHeight: units.gridUnit * 10
    Plasmoid.icon: albumArt ? albumArt : "media-playback-start"
    Plasmoid.toolTipMainText: i18n("No media playing")
    Plasmoid.status: PlasmaCore.Types.ActiveStatus

    Plasmoid.onContextualActionsAboutToShow: {
        plasmoid.clearActions()
        if (canRaise) {
            var icon = mpris2Source.currentData["Desktop Icon Name"] || ""
            plasmoid.setAction("open", i18nc("Open player window or bring it to the front if already open", "Open"), icon)
        }

        if (canControl) {
            plasmoid.setAction("previous", i18nc("Play previous track", "Previous Track"),
                               Qt.application.layoutDirection === Qt.RightToLeft ? "media-skip-forward" : "media-skip-backward");
            plasmoid.action("previous").enabled = Qt.binding(function() {
                return root.canGoPrevious
            })

            if (root.state == "playing") {
                plasmoid.setAction("playPause", i18nc("Pause playback", "Pause"), "media-playback-pause")
            } else {
                plasmoid.setAction("playPause", i18nc("Start playback", "Play"), "media-playback-start")
            }

            plasmoid.setAction("next", i18nc("Play next track", "Next Track"),
                               Qt.application.layoutDirection === Qt.RightToLeft ? "media-skip-backward" : "media-skip-forward")
            plasmoid.action("next").enabled = Qt.binding(function() {
                return root.canGoNext
            })

            plasmoid.setAction("stop", i18nc("Stop playback", "Stop"), "media-playback-stop")
        }

        if (canQuit) {
            plasmoid.setActionSeparator("quitseparator");
            plasmoid.setAction("quit", i18nc("Quit player", "Quit"), "application-exit")
        }
    }

    // HACK Some players like Amarok take quite a while to load the next track
    // this avoids having the plasmoid jump between popup and panel
    onStateChanged: {
        if (state != "") {
            plasmoid.status = PlasmaCore.Types.ActiveStatus
        } else {
            updatePlasmoidStatusTimer.restart()
        }
    }

    Timer {
        id: updatePlasmoidStatusTimer
        interval: 250
        onTriggered: {
            if (state != "") {
                plasmoid.status = PlasmaCore.Types.ActiveStatus
            } else {
                plasmoid.status = PlasmaCore.Types.PassiveStatus
            }
        }
    }

    Plasmoid.fullRepresentation: ExpandedRepresentation {}

    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
        source: root.state === "paused" ? "media-playback-pause" : "media-playback-start"
        active: compactMouse.containsMouse

        MouseArea {
            id: compactMouse
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.BackButton | Qt.ForwardButton
            onClicked: {
                switch (mouse.button) {
                case Qt.MiddleButton:
                    root.action_playPause()
                    break
                case Qt.BackButton:
                    root.action_previous()
                    break
                case Qt.ForwardButton:
                    root.action_next()
                    break
                default:
                    plasmoid.expanded = !plasmoid.expanded
                }
            }
        }
    }

    PlasmaCore.DataSource {
        id: mpris2Source

        readonly property string multiplexSource: "@multiplex"
        property string current: multiplexSource

        readonly property var currentData: data[current]

        engine: "mpris2"
        connectedSources: current

        onSourceRemoved: {
            // if player is closed, reset to multiplex source
            if (source === current) {
                current = multiplexSource
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

    states: [
        State {
            name: "playing"
            when: !root.noPlayer && mpris2Source.currentData.PlaybackStatus === "Playing"

            PropertyChanges {
                target: plasmoid
                icon: albumArt ? albumArt : "media-playback-start"
                toolTipMainText: track
                toolTipSubText: artist ? i18nc("Artist of the song", "by %1", artist) : ""
            }
        },
        State {
            name: "paused"
            when: !root.noPlayer && mpris2Source.currentData.PlaybackStatus === "Paused"

            PropertyChanges {
                target: plasmoid
                icon: albumArt ? albumArt : "media-playback-pause"
                toolTipMainText: track
                toolTipSubText: artist ? i18nc("Artist of the song", "by %1 (paused)", artist) : i18n("Paused")
            }
        }
    ]
}
