/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtQml.Models // DelegateChoice for Qt >= 6.9
import Qt.labs.qmlmodels // DelegateChooser

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
    readonly property bool editing: T.StackView.view.currentItem instanceof EditPage

    property alias view: menuListView
    property alias filter: filter

    background: null
    contentWidth: availableWidth - (contentItem as ListView).leftMargin - (contentItem as ListView).rightMargin

    PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

    onItemSelected: uuid => model.moveToTop(uuid);
    onRemove: uuid => model.remove(uuid)
    onEdit: modelData => {
        clipboardMenu.T.StackView.view.push(Qt.resolvedUrl("EditPage.qml"), {
            dialogItem: clipboardMenu.dialogItem,
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

    onExpandedChanged: {
        if (expanded) {
            menuListView.forceActiveFocus();
        }
    }

    Keys.forwardTo: [clipboardMenu.T.StackView.view.currentItem]
    Keys.onPressed: event => {
        if (clipboardMenu.T.StackView.view.currentItem !== clipboardMenu) {
            event.accepted = false;
            return;
        }

        function forwardToFilter() {
            if (filter.enabled && event.text !== "" && !filter.activeFocus) {
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
            // Let natural KeyNavigation handle Tab/Backtab
            event.accepted = false;
            break;
        }
        case Qt.Key_Backspace: {
            // filter.text += event.text wil break if the key is backspace
            if (!filter.activeFocus) {
                // Forward backspace to filter when not focused
                filter.forceActiveFocus();
                filter.text = filter.text.slice(0, -1);
                event.accepted = true;
            } else {
                // Filter is focused, let the SearchField handle it natively
                event.accepted = false;
            }
            break;
        }
        case Qt.Key_Home: {
            if (menuListView.count > 0) {
                menuListView.currentIndex = 0;
                event.accepted = true;
            } else {
                event.accepted = false;
            }
            break;
        }
        case Qt.Key_End: {
            if (menuListView.count > 0) {
                menuListView.currentIndex = menuListView.count - 1;
                event.accepted = true;
            } else {
                event.accepted = false;
            }
            break;
        }
        case Qt.Key_PageUp: {
            if (event.modifiers & Qt.ControlModifier) {
                // Ctrl+PgUp: Previous tab
                tabBar.setCurrentIndex(Math.max(0, tabBar.currentIndex - 1));
                event.accepted = true;
            } else if (menuListView.count > 0) {
                // Regular PgUp: Navigate list
                menuListView.currentIndex = Math.max(menuListView.currentIndex - pageUpPageDownSkipCount, 0);
                menuListView.positionViewAtIndex(menuListView.currentIndex, ListView.Beginning)
                if (menuListView.currentItem) {
                    menuListView.currentItem.forceActiveFocus()
                }
                event.accepted = true;
            }
            break;
        }
        case Qt.Key_PageDown: {
            if (event.modifiers & Qt.ControlModifier) {
                // Ctrl+PgDn: Next tab
                tabBar.setCurrentIndex(Math.min(tabBar.count - 1, tabBar.currentIndex + 1));
                event.accepted = true;
            } else if (menuListView.count > 0) {
                // Regular PgDn: Navigate list
                menuListView.currentIndex = Math.min(menuListView.currentIndex + pageUpPageDownSkipCount, menuListView.count - 1);
                menuListView.positionViewAtIndex(menuListView.currentIndex, ListView.Beginning)
                if (menuListView.currentItem) {
                    menuListView.currentItem.forceActiveFocus()
                }
                event.accepted = true;
            }
            break;
        }
        case Qt.Key_1:
        case Qt.Key_2: {
            if (event.modifiers & Qt.AltModifier) {
                // Alt+1/Alt+2: Switch to specific tab
                const tabIndex = event.key - Qt.Key_1;
                if (tabIndex < tabBar.count) {
                    tabBar.setCurrentIndex(tabIndex);
                    event.accepted = true;
                }
            } else {
                forwardToFilter();
            }
            break;
        }
        default: {
            // Only forward printable characters to filter
            if (event.text.length > 0 && event.text.charCodeAt(0) >= 32) {
                forwardToFilter();
            } else {
                event.accepted = false;
            }
            break;
        }
        }
    }

    // Hidden PlasmoidHeading for metrics purposes
    PlasmaExtras.PlasmoidHeading {
        id: metricsPlasmoidHeading
        visible: false
    }

    // Hidden metrics ToolButton for delegates to reference implicit sizing
    // Avoids creating one per delegate which would waste memory
    PlasmaComponents3.ToolButton {
        id: metricsStarButton
        visible: false
        display: PlasmaComponents3.AbstractButton.IconOnly
        icon.name: "starred-symbolic"
    }

    // Exported metrics for delegates
    readonly property int starMetricsImplicitWidth: metricsStarButton.implicitWidth
    readonly property int starMetricsImplicitHeight: metricsStarButton.implicitHeight

    property PlasmaExtras.PlasmoidHeading header: PlasmaExtras.PlasmoidHeading {
        bottomPadding: tabBar.visible ? 0 : metricsPlasmoidHeading.bottomPadding
        contentItem: ColumnLayout {
            // No extra spacing between rows; individual rows set their own spacing
            spacing: 0

            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                PlasmaExtras.SearchField {
                    id: filter
                    Layout.fillWidth: true
                    enabled: menuListView.count > 0 || filter.text.length > 0
                    KeyNavigation.up: clipboardMenu.dialogItem.KeyNavigation.up /* ToolBar */
                    KeyNavigation.right: clearHistoryButton.visible ? clearHistoryButton : (tabBar.visible ? tabBar : null)
                    Keys.onDownPressed: event => {
                        if (tabBar.visible) {
                            tabBar.forceActiveFocus();
                        } else {
                            menuListView.forceActiveFocus();
                        }
                        event.accepted = true;
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

                    KeyNavigation.left: filter
                    KeyNavigation.down: tabBar.visible ? tabBar : menuListView

                    onClicked: {
                        clipboardMenu.model.clearHistory();
                        filter.clear();
                    }

                    PlasmaComponents3.ToolTip {
                        text: clearHistoryButton.text
                    }
                }
            }

            PlasmaComponents3.TabBar {
                id: tabBar
                Layout.fillWidth: true
                visible: clipboardMenu.model.starredCount > 0

                // TabBar focus handling
                activeFocusOnTab: true
                Keys.onUpPressed: event => {
                    filter.forceActiveFocus();
                    event.accepted = true;
                }
                Keys.onDownPressed: event => {
                    menuListView.forceActiveFocus();
                    event.accepted = true;
                }
                Keys.onLeftPressed: event => {
                    if (tabBar.currentIndex > 0) {
                        tabBar.currentIndex--;
                    }
                    event.accepted = true;
                }
                Keys.onRightPressed: event => {
                    if (tabBar.currentIndex < tabBar.count - 1) {
                        tabBar.currentIndex++;
                    }
                    event.accepted = true;
                }

                currentIndex: clipboardMenu.model.starredOnly ? 1 : 0
                onCurrentIndexChanged: {
                    clipboardMenu.model.starredOnly = (currentIndex === 1)
                    // Reset selection to first item when switching tabs
                    if (menuListView.count > 0) {
                        menuListView.currentIndex = 0;
                        menuListView.positionViewAtBeginning();
                    }
                }

                PlasmaComponents3.TabButton {
                    text: i18nd("klipper", "All History")
                    Accessible.onPressAction: tabBar.currentIndex = PlasmaComponents3.TabBar.index
                }

                PlasmaComponents3.TabButton {
                    text: i18nd("klipper", "Starred Only")
                    Accessible.onPressAction: tabBar.currentIndex = PlasmaComponents3.TabBar.index
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

    // TODO: Deal with this magic string roleValue here, should be enum from historyitem.h
    DelegateChooser {
        id: chooser
        role: "type"
        DelegateChoice {
            roleValue: "2"
            delegate: TextItemDelegate {
                listMargins: listItemSvg.margins
            }
        }
        DelegateChoice {
            roleValue: "4"
            delegate: ImageItemDelegate  {
                listMargins: listItemSvg.margins
            }
        }
        DelegateChoice {
            roleValue: "8"
            delegate: UrlItemDelegate {
                listMargins: listItemSvg.margins
            }
        }
    }

    contentItem: ListView {
        id: menuListView

        readonly property alias clipboardMenu: clipboardMenu

        highlightFollowsCurrentItem: false
        currentIndex: 0

        // ListView KeyNavigation for when no items have focus
        KeyNavigation.left: tabBar.visible ? tabBar : filter

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
            if (menuListView.currentIndex > 0) {
                menuListView.decrementCurrentIndex();
                menuListView.positionViewAtIndex(menuListView.currentIndex, ListView.Visible);
                event.accepted = true;
            } else {
                // At top of list, or list is empty. Focus TabBar or filter depending on visibility.
                if (tabBar.visible) {
                    tabBar.forceActiveFocus();
                } else {
                    filter.forceActiveFocus();
                }
                event.accepted = true;
            }
        }

        Keys.onDownPressed: event => {
            if (menuListView.currentIndex < menuListView.count - 1) {
                menuListView.incrementCurrentIndex();
                menuListView.positionViewAtIndex(menuListView.currentIndex, ListView.Visible);
                event.accepted = true;
            } else {
                // At bottom of list, or list is empty.
                // If list is not empty, stay here. If empty, let nav happen.
                event.accepted = menuListView.count > 0;
            }
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
