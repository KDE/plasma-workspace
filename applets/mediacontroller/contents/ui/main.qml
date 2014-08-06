/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
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

    property string track: mpris2Source.data[mpris2Source.current].Metadata["xesam:title"]
                           || String(mpris2Source.data[mpris2Source.current].Metadata["xesam:url"]).substring(String(mpris2Source.data[mpris2Source.current].Metadata["xesam:url"]).lastIndexOf("/") + 1)
    property string artist: mpris2Source.data[mpris2Source.current].Metadata["xesam:artist"] || ""
    property string playerIcon: ""

    property bool noPlayer: mpris2Source.sources.length <= 1

    Plasmoid.switchWidth: units.gridUnit * 10
    Plasmoid.switchHeight: units.gridUnit * 8
    Plasmoid.icon: "media-playback-start"
    Plasmoid.fullRepresentation: ExpandedRepresentation {}
    Plasmoid.status: PlasmaCore.Types.PassiveStatus

    state: "off"

    PlasmaCore.DataSource {
        id: mpris2Source
        engine: "mpris2"
        connectedSources: current

        property string current: "@multiplex"
    }

    function playPause() {
        serviceOp(mpris2Source.current, "PlayPause");
    }

    function previous() {
        serviceOp(mpris2Source.current, "Previous");
    }

    function next() {
        serviceOp(mpris2Source.current, "Next");
    }

    function serviceOp(src, op) {
        print(" serviceOp: " + src + " Op: " + op);
        var service = mpris2Source.serviceForSource(src);
        var operation = service.operationDescription(op);
        return service.startOperationCall(operation);
    }

    states: [
        State {
            name: "off"
            when: root.noPlayer
            PropertyChanges { target: plasmoid; status: PlasmaCore.Types.PassiveStatus; }
        },
        State {
            name: "playing"
            when: !root.noPlayer && mpris2Source.data[mpris2Source.current].PlaybackStatus == "Playing"

            PropertyChanges { target: plasmoid; status: PlasmaCore.Types.ActiveStatus; icon: "media-playback-start" }
        },
        State {
            name: "paused"
            when: !root.noPlayer && mpris2Source.data[mpris2Source.current].PlaybackStatus == "Paused"

            PropertyChanges { target: plasmoid; status: PlasmaCore.Types.ActiveStatus; icon: "media-playback-pause" }
        }
    ]
}
