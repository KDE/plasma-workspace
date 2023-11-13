/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQml 2.15
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

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

    contentWidth: availableWidth - contentItem.leftMargin - contentItem.rightMargin

    Keys.onPressed: event => {
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
            target: main
            function onExpandedChanged() {
                if (main.expanded) {
                    menuListView.currentIndex = -1
                    menuListView.positionViewAtBeginning()
                }
            }
        }

        topMargin: Kirigami.Units.largeSpacing
        bottomMargin: Kirigami.Units.largeSpacing
        leftMargin: Kirigami.Units.largeSpacing
        rightMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing

        reuseItems: true

        delegate: ClipboardItemDelegate {
            // FIXME: removing this causes a binding loop
            width: menuListView.width - menuListView.leftMargin - menuListView.rightMargin

            supportsBarcodes: menu.supportsBarcodes

            onItemSelected: uuid => menu.itemSelected(uuid)
            onRemove: uuid => menu.remove(uuid)
            onEdit: uuid => menu.edit(uuid)
            onBarcode: text => menu.barcode(text)
            onTriggerAction: uuid => menu.triggerAction(uuid)

            Binding {
                target: menuListView
                // don't change currentIndex if it would make listview scroll
                // see https://bugs.kde.org/show_bug.cgi?id=387797
                // this is a workaround till https://bugreports.qt.io/browse/QTBUG-114574 gets fixed
                // which would allow a proper solution
                when: hovered && (y - menuListView.contentY + height  + 1 /* border */ < menuListView.height) && (y - menuListView.contentY >= 0)
                property: "currentIndex"; value: index
                restoreMode: Binding.RestoreBinding
            }
        }

        Keys.onUpPressed: event => {
            if (view.currentIndex === 0) {
                view.currentIndex = -1;
                filter.selectAll();
            }
            event.accepted = false; // Forward to KeyNavigation.up
        }

        Loader {
            id: emptyHint

            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.gridUnit * 4)

            active: menuListView.count === 0
            visible: active
            asynchronous: true

            sourceComponent: PlasmaExtras.PlaceholderMessage {
                width: parent.width
                readonly property bool hasText: model.filterRegularExpression.length > 0
                iconName: hasText ? "edit-none" : "edit-paste"
                text: hasText ? i18n("No matches") : i18n("Clipboard is empty")
            }
        }
    }
}
