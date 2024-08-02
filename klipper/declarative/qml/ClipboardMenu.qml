/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import Qt.labs.qmlmodels as QtLabsModel // DelegateChooser

import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.kirigami 2.20 as Kirigami
import org.kde.ksvg 1.0 as KSvg
import org.kde.plasma.private.clipboard 0.1 as Private

PlasmaComponents3.ScrollView {
    id: clipboardMenu

    signal itemSelected(var uuid)
    signal remove(var uuid)
    signal edit(var modelData)
    signal barcode(string text)
    signal triggerAction(var uuid)

    required property bool expanded
    required property PlasmaExtras.Representation dialogItem
    required property Private.HistoryModel model
    required property bool showsClearHistoryButton
    required property string barcodeType

    readonly property int pageUpPageDownSkipCount: menuListView.visibleArea.heightRatio * menuListView.count

    property alias view: menuListView
    property alias filter: filter
    property bool editing: false

    background: null
    contentWidth: availableWidth - (contentItem as ListView).leftMargin - (contentItem as ListView).rightMargin

    onItemSelected: uuid => model.moveToTop(uuid);
    onRemove: uuid => model.remove(uuid)
    onEdit: modelData => {
        clipboardMenu.T.StackView.view.push(Qt.resolvedUrl("EditPage.qml"), {
            dialogItem: clipboardMenu.dialogItem,
            clipboardMenu: clipboardMenu,
            stack: clipboardMenu.T.StackView.view,
            historyModel: clipboardMenu.model,
            modelData: modelData,
        });
    }
    onBarcode: text => {
        clipboardMenu.T.StackView.view.push(Qt.resolvedUrl("BarcodePage.qml"), {
            expanded: Qt.binding(() => clipboardMenu.expanded),
            stack: clipboardMenu.T.StackView.view,
            text: text,
            barcodeType: Qt.binding(() => clipboardMenu.barcodeType),
        });
    }
    onTriggerAction: uuid => model.invokeAction(uuid)

    Keys.forwardTo: [clipboardMenu.T.StackView.view.currentItem]
    Keys.onPressed: event => {
        if (menuListView.count === 0 || clipboardMenu.T.StackView.view.currentItem !== clipboardMenu) {
            event.accepted = false;
            return;
        }

        function forwardToFilter() {
            if (filter.enabled && event.text !== "" && !filter.activeFocus) {
                clipboardMenu.view.currentIndex = -1
                if (event.matches(StandardKey.Paste)) {
                    filter.paste();
                } else {
                    filter.text = "";
                    filter.text += event.text;
                }
                filter.forceActiveFocus();
                event.accepted = true;
            }
        }

        switch(event.key) {
        case Qt.Key_Escape: {
            if (filter.text != "") {
                filter.text = "";
                event.accepted = true;
            }
            break;
        }
        case Qt.Key_F: {
            if (event.modifiers & Qt.ControlModifier) {
                filter.forceActiveFocus();
                filter.selectAll();
                event.accepted = true;
            } else {
                forwardToFilter();
            }
            break;
        }
        case Qt.Key_Tab:
        case Qt.Key_Backtab: {
            // prevent search filter from getting Tab key events
            break;
        }
        case Qt.Key_Backspace: {
            // filter.text += event.text wil break if the key is backspace
            filter.forceActiveFocus();
            filter.text = filter.text.slice(0, -1);
            event.accepted = true;
            break;
        }
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
            forwardToFilter();
            break;
        }
        }
    }

    property PlasmaExtras.PlasmoidHeading header: PlasmaExtras.PlasmoidHeading {
        focus: true

        contentItem: RowLayout {
            enabled: menuListView.count > 0 || filter.text.length > 0

            PlasmaExtras.SearchField {
                id: filter
                Layout.fillWidth: true

                focus: !Kirigami.InputMethod.willShowOnActive

                KeyNavigation.up: clipboardMenu.dialogItem.KeyNavigation.up /* ToolBar */
                KeyNavigation.down: menuListView.count > 0 ? menuListView : null
                KeyNavigation.right: clearHistoryButton.visible ? clearHistoryButton : null
                Keys.onDownPressed: event => {
                    clipboardMenu.view.incrementCurrentIndex();
                    event.accepted = false;
                }
                Keys.onEnterPressed: event => Keys.returnPressed(event)
                Keys.onReturnPressed: event => {
                    if (menuListView.currentItem !== null) {
                        menuListView.currentItem.Keys.returnPressed(event);
                    } else if (menuListView.count > 0) {
                        menuListView.itemAtIndex(0).Keys.returnPressed(event);
                    } else {
                        event.accepted = false;
                    }
                }
            }

            PlasmaComponents3.ToolButton {
                id: clearHistoryButton
                visible: clipboardMenu.showsClearHistoryButton

                icon.name: "edit-clear-history"

                display: PlasmaComponents3.AbstractButton.IconOnly
                text: i18nd("klipper", "Clear History")

                onClicked: {
                    clipboardMenu.model.clearHistory();
                    filter.clear();
                }

                PlasmaComponents3.ToolTip {
                    text: clearHistoryButton.text
                }
            }
        }
    }

    KSvg.FrameSvgItem {
        id: listItemSvg
        imagePath: "widgets/listitem"
        prefix: "normal"
        visible: false
    }

    QtLabsModel.DelegateChooser {
        id: chooser
        role: "type"
        QtLabsModel.DelegateChoice {
            roleValue: "0"
            delegate: TextItemDelegate {
                listMargins: listItemSvg.margins
            }
        }
        QtLabsModel.DelegateChoice {
            roleValue: "1"
            delegate: ImageItemDelegate  {
                listMargins: listItemSvg.margins
            }
        }
        QtLabsModel.DelegateChoice {
            roleValue: "2"
            delegate: UrlItemDelegate {
                listMargins: listItemSvg.margins
            }
        }
    }

    contentItem: ListView {
        id: menuListView

        readonly property alias clipboardMenu: clipboardMenu

        highlight: PlasmaExtras.Highlight { }
        highlightMoveDuration: 0
        highlightResizeDuration: 0
        currentIndex: 0
        model: KItemModels.KSortFilterProxyModel {
            sourceModel: clipboardMenu.model
            filterRoleName: "display"
            filterRegularExpression: RegExp(filter.text, "i")
        }

        topMargin: Kirigami.Units.largeSpacing
        bottomMargin: Kirigami.Units.largeSpacing
        leftMargin: Kirigami.Units.largeSpacing
        rightMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing

        reuseItems: true

        delegate: chooser

        Keys.onUpPressed: event => {
            if (menuListView.currentIndex === 0) {
                menuListView.currentIndex = -1;
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
                readonly property bool hasText: filter.text.length > 0
                iconName: hasText ? "edit-none" : "edit-paste"
                text: hasText ? i18nd("klipper", "No matches") : i18nd("klipper", "Clipboard is empty")
            }
        }
    }
}
