/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQml 2.15
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

import org.kde.kirigami 2.12 as Kirigami

PlasmaComponents3.ScrollView {
    id: menu

    property alias view: menuListView
    property alias model: menuListView.model
    property bool supportsBarcodes
    readonly property int pageUpPageDownSkipCount: menuListView.visibleArea.heightRatio * menuListView.count

    background: null

    signal itemSelected(string uuid)
    signal remove(string uuid)
    signal edit(string uuid)
    signal barcode(string text)
    signal triggerAction(string uuid)

    // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
    PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

    contentWidth: availableWidth - contentItem.leftMargin - contentItem.rightMargin

    Keys.onPressed: {
        if (menuListView.count !== 0) {
            switch (event.key) {
                case Qt.Key_Home: {
                    menuListView.currentIndex = 0;
                    event.accepted = true;
                    break;
                }
                case Qt.Key_End: {
                    menuListView.currentIndex = menuListView.count - 1;
                    event.accepted = true;
                    break;
                }
                case Qt.Key_PageUp: {
                    menuListView.currentIndex = Math.max(menuListView.currentIndex - pageUpPageDownSkipCount, 0);
                    event.accepted = true;
                    break;
                }
                case Qt.Key_PageDown: {
                    menuListView.currentIndex = Math.min(menuListView.currentIndex + pageUpPageDownSkipCount, menuListView.count - 1);
                    event.accepted = true;
                    break;
                }
                default: {
                    event.accepted = false;
                    break;
                }
            }
        }
    }

    contentItem: ListView {
        id: menuListView

        highlight: PlasmaExtras.Highlight { }
        highlightMoveDuration: 0
        highlightResizeDuration: 0
        currentIndex: -1

        Connections {
            target: plasmoid
            function onExpandedChanged() {
                if (plasmoid.expanded) {
                    menuListView.currentIndex = -1
                    menuListView.positionViewAtBeginning()
                }
            }
        }

        topMargin: PlasmaCore.Units.smallSpacing * 2
        bottomMargin: PlasmaCore.Units.smallSpacing * 2
        leftMargin: PlasmaCore.Units.smallSpacing * 2
        rightMargin: PlasmaCore.Units.smallSpacing * 2
        spacing: PlasmaCore.Units.smallSpacing

        reuseItems: true

        delegate: ClipboardItemDelegate {
            // FIXME: removing this causes a binding loop
            width: menuListView.width - menuListView.leftMargin - menuListView.rightMargin

            supportsBarcodes: menu.supportsBarcodes

            onItemSelected: menu.itemSelected(uuid)
            onRemove: menu.remove(uuid)
            onEdit: menu.edit(uuid)
            onBarcode: menu.barcode(text)
            onTriggerAction: menu.triggerAction(uuid)

            Binding {
                target: menuListView; when: hovered
                property: "currentIndex"; value: index
                restoreMode: Binding.RestoreBinding
            }
        }

        Loader {
            id: emptyHint

            anchors.centerIn: parent
            width: parent.width - (PlasmaCore.Units.largeSpacing * 4)

            active: menuListView.count === 0
            visible: active
            asynchronous: true

            sourceComponent: PlasmaExtras.PlaceholderMessage {
                width: parent.width
                readonly property bool hasText: model.filterRegExp.length > 0
                iconName: hasText ? "edit-none" : "edit-paste"
                text: hasText ? i18n("No matches") : i18n("Clipboard is empty")
            }
        }
    }
}
