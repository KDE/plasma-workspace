/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.15 as QQC2 // For StackView
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core as PlasmaCore
import org.kde.ksvg 1.0 as KSvg
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami

import org.kde.plasma.private.clipboard 0.1 as Private

PlasmoidItem {
    id: main

    readonly property bool isClipboardEmpty: historyModel.count === 0

    switchWidth: Kirigami.Units.gridUnit * 5
    switchHeight: Kirigami.Units.gridUnit * 5
    Plasmoid.status: isClipboardEmpty ? PlasmaCore.Types.PassiveStatus : PlasmaCore.Types.ActiveStatus
    toolTipMainText: i18n("Clipboard Contents")
    toolTipSubText: isClipboardEmpty ? i18n("Clipboard is empty") : historyModel.currentText
    toolTipTextFormat: Text.PlainText
    Plasmoid.icon: "klipper-symbolic"

    function action_configure() {
        klipper.configure();
    }

    function action_clearHistory() {
        historyModel.clearHistory();
        clearSearchField()
    }

    onIsClipboardEmptyChanged: {
        if (isClipboardEmpty) {
            // We need to hide the applet before changing its status to passive
            // because only the active applet can hide itself
            if (main.hideOnWindowDeactivate)
                main.expanded = false;
            Plasmoid.status = PlasmaCore.Types.HiddenStatus;
        } else {
            Plasmoid.status = PlasmaCore.Types.ActiveStatus
        }
    }

    function clearSearchField() {
        (fullRepresentationItem.clipboardMenu as Private.ClipboardMenu).filter.clear();
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            id: clearAction
            text: i18n("Clear History")
            icon.name: "edit-clear-history"
            visible: !main.isClipboardEmpty && !(main.fullRepresentationItem?.clipboardMenu as Private.ClipboardMenu)?.editing
            onTriggered: {
                historyModel.clearHistory();
                main.clearSearchField()
            }
        }
    ]

    PlasmaCore.Action {
        id: configureAction
        text: i18n("Configure Clipboard…")
        icon.name: "configure"
        shortcut: "alt+d, s"
        onTriggered: klipper.configure()
    }

    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction);
    }

    Private.KlipperInterface {
        id: klipper
    }

    Private.HistoryModel {
        id: historyModel
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

        readonly property var appletInterface: main
        readonly property alias clipboardMenu: stack.initialItem // Required to let the outside world access the property

        Keys.forwardTo: [stack.currentItem]

        Connections {
            target: main
            function onExpandedChanged(expanded) {
                if (expanded) {
                    ((stack.initialItem as Private.ClipboardMenu).view as ListView).currentIndex = -1;
                    ((stack.initialItem as Private.ClipboardMenu).view as ListView).positionViewAtBeginning();
                }
            }
        }

        QQC2.StackView {
            id: stack
            anchors.fill: parent
            initialItem: Private.ClipboardMenu {
                expanded: main.expanded
                dialogItem: dialogItem
                model: historyModel
                showsClearHistoryButton: !(Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading) && clearAction.visible
                barcodeType: Plasmoid.configuration.barcodeType

                onItemSelected: if (main.hideOnWindowDeactivate) {
                    main.expanded = false;
                }
            }
        }
    }
}
