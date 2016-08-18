/*
 * Copyright 2014 Marco Martin <mart@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.milou 0.1 as Milou

ColumnLayout {
    id: root
    property string query
    property string runner
    property bool showHistory: false

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    onQueryChanged: {
        queryField.text = query;
    }

    Connections {
        target: runnerWindow
        onVisibleChanged: {
            if (runnerWindow.visible) {
                queryField.forceActiveFocus();
                listView.currentIndex = -1
            } else {
                root.query = "";
                root.runner = ""
                root.showHistory = false
            }
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignTop
        PlasmaComponents.ToolButton {
            iconSource: "configure"
            onClicked: {
                runnerWindow.visible = false
                runnerWindow.displayConfiguration()
            }
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Configure")
            Accessible.description: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Configure Search Plugins")
        }
        PlasmaComponents.TextField {
            id: queryField
            property bool allowCompletion: false

            clearButtonShown: true
            Layout.minimumWidth: units.gridUnit * 25

            activeFocusOnPress: true
            placeholderText: results.runnerName ? i18ndc("plasma_lookandfeel_org.kde.lookandfeel",
                                                         "Textfield placeholder text, query specific KRunner",
                                                         "Search '%1'...", results.runnerName)
                                                : i18ndc("plasma_lookandfeel_org.kde.lookandfeel",
                                                         "Textfield placeholder text", "Search...")

            onTextChanged: {
                root.query = queryField.text
                if (allowCompletion && length > 0) {
                    var history = runnerWindow.history

                    // search the first item in the history rather than the shortest matching one
                    // this way more recently used entries take precedence over older ones (Bug 358985)
                    for (var i = 0, j = history.length; i < j; ++i) {
                        var item = history[i]

                        if (item.toLowerCase().indexOf(text.toLowerCase()) === 0) {
                            var oldText = text
                            text = text + item.substr(oldText.length)
                            select(text.length, oldText.length)
                            break
                        }
                    }
                }
            }
            Keys.onPressed: allowCompletion = (event.key !== Qt.Key_Backspace && event.key !== Qt.Key_Delete)
            Keys.onUpPressed: {
                if (length === 0) {
                    root.showHistory = true
                }
            }
            Keys.onDownPressed: {
                if (length === 0) {
                    root.showHistory = true
                }
            }

            Keys.onEscapePressed: {
                runnerWindow.visible = false
            }
            Keys.forwardTo: [listView, results]
        }
        PlasmaComponents.ToolButton {
            iconSource: "window-close"
            onClicked: runnerWindow.visible = false
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Close")
            Accessible.description: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Close Search")
        }
    }

    PlasmaExtras.ScrollArea {
        Layout.alignment: Qt.AlignTop
        visible: results.count > 0
        enabled: visible
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(Screen.height, results.contentHeight)

        Milou.ResultsView {
            id: results
            queryString: root.query
            runner: root.runner

            onActivated: {
                runnerWindow.addToHistory(queryString)
                runnerWindow.visible = false
            }

            onUpdateQueryString: {
                queryField.text = text
                queryField.cursorPosition = cursorPosition
            }
        }
    }

    PlasmaExtras.ScrollArea {
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        visible: root.query.length === 0 && listView.count > 0
        // don't accept keyboard input when not visible so the keys propagate to the other list
        enabled: visible
        Layout.preferredHeight: Math.min(Screen.height, listView.contentHeight)

        ListView {
            id: listView // needs this id so the delegate can access it
            keyNavigationWraps: true
            highlight: PlasmaComponents.Highlight {}
            highlightMoveDuration: 0
            // we store 50 entries in the history but only show 20 in the UI so it doesn't get too huge
            model: root.showHistory ? runnerWindow.history.slice(0, 20) : []
            delegate: Milou.ResultDelegate {
                id: resultDelegate
                width: listView.width
                typeText: index === 0 ? i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Recent Queries") : ""
                additionalActions: [{
                    icon: "list-remove",
                    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Remove")
                }]
            }

            Keys.onReturnPressed: runCurrentIndex()
            Keys.onEnterPressed: runCurrentIndex()

            Keys.onTabPressed: incrementCurrentIndex()
            Keys.onBacktabPressed: decrementCurrentIndex()
            Keys.onUpPressed: decrementCurrentIndex()
            Keys.onDownPressed: incrementCurrentIndex()

            function runCurrentIndex() {
                var entry = runnerWindow.history[currentIndex]
                if (entry) {
                    queryField.text = entry
                }
            }

            function runAction(actionIndex) {
                if (actionIndex === 0) {
                    // QStringList changes just reset the model, so we'll remember the index and set it again
                    var currentIndex = listView.currentIndex
                    runnerWindow.removeFromHistory(currentIndex)
                    listView.currentIndex = currentIndex
                }
            }
        }

    }
}
