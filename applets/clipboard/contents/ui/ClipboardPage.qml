/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.4
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

Menu {
    id: clipboardMenu
    Keys.onPressed: {
        function forwardToFilter() {
            if (event.text !== "" && !filter.activeFocus) {
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
            case Qt.Key_Enter:
            case Qt.Key_Return: {
                if (clipboardMenu.view.currentIndex >= 0) {
                    var uuid = clipboardMenu.model.get(clipboardMenu.view.currentIndex).UuidRole
                    if (uuid) {
                        clipboardSource.service(uuid, "select")
                        if (plasmoid.hideOnWindowDeactivate) {
                            plasmoid.expanded = false;
                        }
                    }
                }
                break;
            }
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
        RowLayout {
            anchors.fill: parent
            enabled: clipboardMenu.model.count > 0 || filter.text.length > 0

            PlasmaComponents3.TextField {
                id: filter
                placeholderText: i18n("Search…")
                clearButtonShown: true
                Layout.fillWidth: true

                inputMethodHints: Qt.ImhNoPredictiveText

                Keys.onUpPressed: clipboardMenu.arrowKeyPressed(event)
                Keys.onDownPressed: clipboardMenu.arrowKeyPressed(event)

                Connections {
                    target: main
                    function onClearSearchField() {
                        filter.clear()
                    }
                }
            }
            PlasmaComponents3.ToolButton {
                visible: !(plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading) && plasmoid.action("clearHistory").visible

                icon.name: "edit-clear-history"
                onClicked: {
                    clipboardSource.service("", "clearHistory")
                    filter.clear()
                }

                PlasmaComponents3.ToolTip {
                    text: i18n("Clear history")
                }
            }
        }
    }

    model: PlasmaCore.SortFilterModel {
        sourceModel: clipboardSource.models.clipboard
        filterRole: "FullTextRole"
        filterRegExp: filter.text
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
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.topMargin: PlasmaCore.Units.smallSpacing
    onItemSelected: clipboardSource.service(uuid, "select")
    onRemove: clipboardSource.service(uuid, "remove")
    onEdit: {
        stack.push(Qt.resolvedUrl("EditPage.qml"), {
            text: clipboardMenu.model.get(clipboardMenu.view.currentIndex).FullTextRole,
            uuid: uuid
        });
    }
    onBarcode: {
        stack.push(Qt.resolvedUrl("BarcodePage.qml"), {
            text: text
        });
    }
    onAction: {
        clipboardSource.service(uuid, "action")
        clipboardMenu.view.currentIndex = 0
    }

    Component.onCompleted: {
        // Intercept up/down key to prevent ListView from accepting the key event.
        clipboardMenu.view.Keys.upPressed.connect(clipboardMenu.arrowKeyPressed);
        clipboardMenu.view.Keys.downPressed.connect(clipboardMenu.arrowKeyPressed);

        // Focus on the search field when the applet is opened for the first time
        filter.forceActiveFocus();
    }

    function goToCurrent() {
        clipboardMenu.view.positionViewAtIndex(clipboardMenu.view.currentIndex, ListView.Contain);
        if (clipboardMenu.view.currentIndex !== -1) {
            clipboardMenu.view.currentItem.forceActiveFocus();
        }
    }

    function arrowKeyPressed(event) {
        switch (event.key) {
        case Qt.Key_Up: {
            if (clipboardMenu.view.currentIndex === 0) {
                clipboardMenu.view.currentIndex = -1;
                filter.forceActiveFocus();
                filter.selectAll();
            } else {
                clipboardMenu.view.decrementCurrentIndex();
                goToCurrent();
            }
            event.accepted = true;
            break;
        }
        case Qt.Key_Down: {
            clipboardMenu.view.incrementCurrentIndex();
            goToCurrent();
            event.accepted = true;
            break;
        }
        default:
            break;
        }
    }
}
