/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.15 as QQC2 // For StackView
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: main

    property bool isClipboardEmpty: clipboardSource.data["clipboard"]["empty"]
    property bool editing: false

    signal clearSearchField

    Plasmoid.switchWidth: PlasmaCore.Units.gridUnit * 5
    Plasmoid.switchHeight: PlasmaCore.Units.gridUnit * 5
    Plasmoid.status: isClipboardEmpty ? PlasmaCore.Types.PassiveStatus : PlasmaCore.Types.ActiveStatus
    Plasmoid.toolTipMainText: i18n("Clipboard Contents")
    Plasmoid.toolTipSubText: isClipboardEmpty ? i18n("Clipboard is empty") : clipboardSource.data["clipboard"]["current"]
    Plasmoid.toolTipTextFormat: Text.PlainText
    Plasmoid.icon: "klipper"

    function action_configure() {
        clipboardSource.service("", "configureKlipper");
    }

    function action_clearHistory() {
        clipboardSource.service("", "clearHistory")
        clearSearchField()
    }

    onIsClipboardEmptyChanged: {
        if (isClipboardEmpty) {
            // We need to hide the applet before changing its status to passive
            // because only the active applet can hide itself
            if (Plasmoid.hideOnWindowDeactivate)
                Plasmoid.expanded = false;
            Plasmoid.status = PlasmaCore.Types.PassiveStatus;
        } else {
            Plasmoid.status = PlasmaCore.Types.ActiveStatus
        }
    }


    Component.onCompleted: {
        Plasmoid.removeAction("configure");
        Plasmoid.setAction("configure", i18n("Configure Clipboard…"), "configure", "alt+d, s");

        Plasmoid.setAction("clearHistory", i18n("Clear History"), "edit-clear-history");
        Plasmoid.action("clearHistory").visible = Qt.binding(() => {
            return !main.isClipboardEmpty && !main.editing;
        });
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
        function edit(uuid, text) {
            clipboardSource.editing = true;
            const service = clipboardSource.serviceForSource(uuid);
            const operation = service.operationDescription("edit");
            operation.text = text;
            const job = service.startOperationCall(operation);
            job.finished.connect(function() {
                clipboardSource.editing = false;
            });
        }
    }

    Plasmoid.fullRepresentation: PlasmaExtras.Representation {
        id: dialogItem
        Layout.minimumWidth: PlasmaCore.Units.gridUnit * 24
        Layout.minimumHeight: PlasmaCore.Units.gridUnit * 24
        Layout.maximumWidth: PlasmaCore.Units.gridUnit * 80
        Layout.maximumHeight: PlasmaCore.Units.gridUnit * 40
        collapseMarginsHint: true

        focus: true

        header: stack.currentItem.header

        property alias listMargins: listItemSvg.margins
        readonly property var appletInterface: Plasmoid.self

        PlasmaCore.FrameSvgItem {
            id : listItemSvg
            imagePath: "widgets/listitem"
            prefix: "normal"
            visible: false
        }

        Keys.forwardTo: [stack.currentItem]

        QQC2.StackView {
            id: stack
            anchors.fill: parent
            initialItem: ClipboardPage {}
        }
    }
}
