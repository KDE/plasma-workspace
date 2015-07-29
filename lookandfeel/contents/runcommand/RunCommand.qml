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

            onTextChanged: {
                root.query = queryField.text
                if (allowCompletion && length > 0) {
                    var history = runnerWindow.history
                    var candidate = ""
                    var shortest = ""

                    for (var i = 0, j = history.length; i < j; ++i) {
                        var item = history[i]

                        if (item.toLowerCase().indexOf(text.toLowerCase()) === 0) {
                            if (candidate.length > 0) {
                                if (item.length < candidate.length) {
                                    candidate = item
                                }

                                shortest = shortest.substring(0, item.length, shortest.length)
                            } else {
                                candidate = item
                                shortest = item
                            }
                        }
                    }

                    if (candidate.length > 0) {
                        var oldText = text
                        text = candidate
                        select(text.length, oldText.length)
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
            }

            Keys.onReturnPressed: runCurrentIndex()
            Keys.onEnterPressed: runCurrentIndex()

            Keys.onTabPressed: incrementCurrentIndex()
            Keys.onBacktabPressed: decrementCurrentIndex()
            Keys.onUpPressed: decrementCurrentIndex()
            Keys.onDownPressed: incrementCurrentIndex()

            function runCurrentIndex() {
                queryField.text = runnerWindow.history[currentIndex]
            }
        }

    }
}
