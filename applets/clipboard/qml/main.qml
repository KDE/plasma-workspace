/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2 // For StackView
import QtQuick.Templates as T
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.ksvg as KSvg
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

import org.kde.plasma.private.clipboard as Private

PlasmoidItem {
    id: main

    readonly property bool isClipboardEmpty: historyModel.sourceCount === 0

    switchWidth: Kirigami.Units.gridUnit * 12
    switchHeight: Kirigami.Units.gridUnit * 12
    Plasmoid.status: isClipboardEmpty ? PlasmaCore.Types.PassiveStatus : PlasmaCore.Types.ActiveStatus
    toolTipMainText: i18n("Clipboard Contents")
    toolTipSubText: if (isClipboardEmpty) {
        i18n("Clipboard is empty")
    } else if (historyModel.sourceModel.hasPassword) {
        i18nc("@info", "The clipboard currently holds sensitive data")
    } else {
        historyModel.currentText
    }
    toolTipTextFormat: Text.PlainText
    Plasmoid.icon: "klipper-symbolic"

    readonly property Private.ClipboardMenu clipboardMenu: main.fullRepresentationItem?.clipboardMenu as Private.ClipboardMenu

    function action_configure() {
        klipper.configure();
    }

    function action_clearHistory() {
        main.clipboardMenu.clearHistory()
    }

    // BUG 520144
    // QTBUG-146886
    readonly property var backAction: Kirigami.Action {
        enabled: main.clipboardMenu?.T.StackView.view.depth === 2
        onTriggered: main.clipboardMenu.closeBarcode()
    }

    property bool inEmbeddedContainment: Plasmoid.containment.containmentType === PlasmaCore.Containment.CustomEmbedded

    onIsClipboardEmptyChanged: {
        if (isClipboardEmpty) {
            if (!main.inEmbeddedContainment)
                return;
            // We need to hide the applet before changing its status to passive
            // because only the active applet can hide itself
            if (main.hideOnWindowDeactivate)
                main.expanded = false;
            Plasmoid.status = PlasmaCore.Types.HiddenStatus;
        } else {
            Plasmoid.status = PlasmaCore.Types.ActiveStatus
        }
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            id: clearAction
            text: i18n("Clear History")
            icon.name: "edit-clear-history"
            visible: !main.isClipboardEmpty && !main.clipboardMenu?.editing && !copyClipboardAction.visible
            onTriggered: main.clipboardMenu.clearHistory()
        },
        PlasmaCore.Action {
            id: copyClipboardAction

            readonly property var page: main.clipboardMenu?.T.StackView.view.currentItem

            icon.name: page?.copyAction?.icon.name ?? ""
            text: page?.copyAction?.tooltip ?? ""
            onTriggered: page.copyAction.triggered()
            enabled: page?.copyAction?.enabled ?? ""
            visible: Plasmoid.containment.pluginName === "org.kde.plasma.systemtray" && page instanceof Private.BarcodePage
        }
    ]

    PlasmaCore.Action {
        id: configureAction
        text: i18n("Configure Clipboard…")
        icon.name: "configure"
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
                    (menu.view as ListView).currentIndex = -1;
                    (menu.view as ListView).positionViewAtBeginning();
                } else {
                    menu.clearFilter();
                }
            }
        }

        QQC2.StackView {
            id: stack
            anchors.fill: parent
            initialItem: Private.ClipboardMenu {
                id: menu
                expanded: main.expanded
                dialogItem: dialogItem
                model: historyModel
                showsClearHistoryButton: !(Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading) && clearAction.visible
                keyboardActivated: false
                barcodeType: Plasmoid.configuration.barcodeType
                showHeader: Plasmoid.containment.pluginName !== "org.kde.plasma.systemtray"

                onItemSelected: if (main.hideOnWindowDeactivate) {
                    main.expanded = false;
                }
            }
        }
    }
}
