/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.4
import QtQuick.Controls as QQC
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kitemmodels 1.0 as KItemModels

import org.kde.kirigami 2.19 as Kirigami // for InputMethod.willShowOnActive

Menu {
    id: clipboardMenu

    Keys.onPressed: event => {
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
        if (stack.currentItem !== clipboardMenu) {
            event.accepted = false;
            return;
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
            default: {
                forwardToFilter();
            }
        }
    }

    Keys.forwardTo: [stack.currentItem]

    property var header: PlasmaExtras.PlasmoidHeading {
        // This uses expanded to ensure the binding gets reevaluated
        // when the plasmoid is shown again and that way ensure we are
        // always in the correct state on show.
        focus: main.expanded

        contentItem: RowLayout {
            enabled: clipboardMenu.model.count > 0 || filter.text.length > 0

            PlasmaExtras.SearchField {
                id: filter
                Layout.fillWidth: true

                focus: !Kirigami.InputMethod.willShowOnActive

                KeyNavigation.up: dialogItem.KeyNavigation.up /* ToolBar */
                KeyNavigation.down: clipboardMenu.contentItem.count > 0 ? clipboardMenu.contentItem /* ListView */ : null
                KeyNavigation.right: clearHistoryButton.visible ? clearHistoryButton : null
                Keys.onDownPressed: event => {
                    clipboardMenu.view.incrementCurrentIndex();
                    event.accepted = false;
                }

                Connections {
                    target: main
                    function onClearSearchField() {
                        filter.clear()
                    }
                }
            }
            PlasmaComponents3.ToolButton {
                id: clearHistoryButton
                visible: !(Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading) && main.clearHistoryAction.visible

                icon.name: "edit-clear-history"

                display: PlasmaComponents3.AbstractButton.IconOnly
                text: main.clearHistoryAction.text

                onClicked: {
                    clipboardSource.service("", "clearHistory")
                    filter.clear()
                }

                PlasmaComponents3.ToolTip {
                    text: parent.text
                }
            }
        }
    }

    model: KItemModels.KSortFilterProxyModel {
        sourceModel: clipboardSource.models.clipboard
        filterRoleName: "DisplayRole"
        filterRegularExpression: RegExp(filter.text, "i")
    }
    supportsBarcodes: {
        try {
            let prisonTest = Qt.createQmlObject("import QtQml 2.0; import org.kde.prison 1.0; QtObject {}", this);
            prisonTest.destroy();
        } catch (e) {
            console.log("Barcodes not supported:", e);
            return false;
        }
        return true;
    }
    onItemSelected: uuid => clipboardSource.service(uuid, "select")
    onRemove: uuid => clipboardSource.service(uuid, "remove")
    onEdit: uuid => {
        const m = clipboardMenu.model;
        const index = m.index(clipboardMenu.view.currentIndex, 0);
        const text = m.data(index, Qt.DisplayRole);
        stack.push(Qt.resolvedUrl("EditPage.qml"), { text, uuid });
    }
    onBarcode: text => {
        stack.push(Qt.resolvedUrl("BarcodePage.qml"), {
            text: text
        });
    }
    onTriggerAction: uuid => clipboardSource.service(uuid, "action")
}
