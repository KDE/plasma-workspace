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
import org.kde.ksvg 1.0 as KSvg
import org.kde.plasma.plasma5support 2.0 as P5Support
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami

PlasmoidItem {
    id: main

    property bool isClipboardEmpty: clipboardSource.data["clipboard"]["empty"]
    property bool editing: false
    property alias clearHistoryAction: clearAction

    signal clearSearchField

    switchWidth: Kirigami.Units.gridUnit * 5
    switchHeight: Kirigami.Units.gridUnit * 5
    Plasmoid.status: isClipboardEmpty ? PlasmaCore.Types.PassiveStatus : PlasmaCore.Types.ActiveStatus
    toolTipMainText: i18n("Clipboard Contents")
    toolTipSubText: isClipboardEmpty ? i18n("Clipboard is empty") : clipboardSource.data["clipboard"]["current"]
    toolTipTextFormat: Text.PlainText
    Plasmoid.icon: "klipper-symbolic"

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
            if (main.hideOnWindowDeactivate)
                main.expanded = false;
            Plasmoid.status = PlasmaCore.Types.PassiveStatus;
        } else {
            Plasmoid.status = PlasmaCore.Types.ActiveStatus
        }
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            id: clearAction
            text: i18n("Clear History")
            icon.name: "edit-clear-history"
            visible: !main.isClipboardEmpty && !main.editing
            onTriggered: {
                clipboardSource.service("", "clearHistory")
                clearSearchField()
            }
        }
    ]

    PlasmaCore.Action {
        id: configureAction
        text: i18n("Configure Clipboard…")
        icon.name: "configure"
        shortcut: "alt+d, s"
        onTriggered: {
            clipboardSource.service("", "configureKlipper")
        }
    }

    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction);
    }

    P5Support.DataSource {
        id: clipboardSource
        property bool editing: false;
        engine: "clipboard"
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

    fullRepresentation: PlasmaExtras.Representation {
        id: dialogItem
        Layout.minimumWidth: Kirigami.Units.gridUnit * 24
        Layout.minimumHeight: Kirigami.Units.gridUnit * 24
        Layout.maximumWidth: Kirigami.Units.gridUnit * 80
        Layout.maximumHeight: Kirigami.Units.gridUnit * 40
        collapseMarginsHint: true

        focus: true

        header: stack.currentItem.header

        property alias listMargins: listItemSvg.margins
        readonly property var appletInterface: main

        KSvg.FrameSvgItem {
            id : listItemSvg
            imagePath: "widgets/listitem"
            prefix: "normal"
            visible: false
        }

        Keys.forwardTo: [stack.currentItem]

        QQC2.StackView {
            id: stack
            anchors.fill: parent
            initialItem: ClipboardPage {
            }
        }
    }
}
