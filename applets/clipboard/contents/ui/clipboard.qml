/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright (C) 2014 Kai Uwe Broulik <kde@privat.broulik.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: main

    Plasmoid.switchWidth: units.gridUnit * 5
    Plasmoid.switchHeight: units.gridUnit * 5
    Plasmoid.status: clipboardSource.data["clipboard"]["empty"] ? PlasmaCore.Types.PassiveStatus : PlasmaCore.Types.ActiveStatus
    Plasmoid.toolTipMainText: i18n("Clipboard Contents")
    Plasmoid.toolTipSubText: clipboardSource.data["clipboard"]["empty"] ? i18n("Clipboard is empty") : clipboardSource.data["clipboard"]["current"]
    Plasmoid.toolTipTextFormat: Text.PlainText
    Plasmoid.icon: "klipper"

    function action_configure() {
        clipboardSource.service("", "configureKlipper");
    }

    Component.onCompleted: {
        plasmoid.removeAction("configure");
        plasmoid.setAction("configure", i18n("Configure Clipboard..."), "configure", "alt+d, s");
    }

    PlasmaCore.DataSource {
        id: clipboardSource
        property bool editing: false;
        engine: "org.kde.plasma.clipboard"
        connectedSources: "clipboard"
        function service(uuid, op) {
            var service = clipboardSource.serviceForSource(uuid);
            var operation = service.operationDescription(op);
            return service.startOperationCall(operation);
        }
        function edit(uuid) {
            clipboardSource.editing = true;
            var job = clipboardSource.service(uuid, "edit");
            job.finished.connect(function() {
                clipboardSource.editing = false;
            });
        }
    }

    Plasmoid.fullRepresentation: Item {
        id: dialogItem
        Layout.minimumWidth: units.gridUnit * 5
        Layout.minimumHeight: units.gridUnit * 5

        focus: true

        property alias listMargins: listItemSvg.margins

        PlasmaCore.FrameSvgItem {
            id : listItemSvg
            imagePath: "widgets/listitem"
            prefix: "normal"
            visible: false
        }

        Keys.forwardTo: [stack.currentPage]

        PlasmaComponents.PageStack {
            id: stack
            anchors.fill: parent
            initialPage: ClipboardPage {
                anchors.fill: parent
            }
        }
        Component {
            id: barcodePage
            BarcodePage {
                anchors.fill: parent
            }
        }
    }
}
