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

import QtQuick 2.6
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
            visible: runnerWindow.canConfigure
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
                    root.showHistory = true;
                    listView.forceActiveFocus();
                } else if (results.count > 0) {
                    results.forceActiveFocus();
                    results.decrementCurrentIndex();
                }
            }
            Keys.onDownPressed: {
                if (length === 0) {
                    root.showHistory = true;
                    listView.forceActiveFocus();
                } else if (results.count > 0) {
                    results.forceActiveFocus();
                    results.incrementCurrentIndex();
                }
            }
            Keys.onEnterPressed: results.runCurrentIndex(event)
            Keys.onReturnPressed: results.runCurrentIndex(event)

            Keys.onEscapePressed: {
                runnerWindow.visible = false
            }

            PlasmaCore.SvgItem {
                anchors {
                    right: parent.right
                    rightMargin: 6 // from PlasmaStyle TextFieldStyle
                    verticalCenter: parent.verticalCenter
                }
                // match clear button
                width: Math.max(parent.height * 0.8, units.iconSizes.small)
                height: width
                svg: PlasmaCore.Svg {
                    imagePath: "widgets/arrows"
                    colorGroup: PlasmaCore.Theme.ButtonColorGroup
                }
                elementId: "down-arrow"
                visible: queryField.length === 0

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        root.showHistory = !root.showHistory
                        if (root.showHistory) {
                            listView.forceActiveFocus(); // is the history list
                        } else {
                            queryField.forceActiveFocus();
                        }
                    }
                }
            }
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

            Keys.onPressed: {
                if (event.text != "") {
                    queryField.text += event.text;
                    queryField.focus = true;
                }
            }

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
            activeFocusOnTab: true
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
                Accessible.description: i18n("in category recent queries")
            }

            onActiveFocusChanged: {
                if (!activeFocus && currentIndex == listView.count-1) {
                    currentIndex = 0;
                }
            }
            Keys.onReturnPressed: runCurrentIndex()
            Keys.onEnterPressed: runCurrentIndex()
            
            Keys.onTabPressed: {
                if (currentIndex == listView.count-1) {
                    listView.nextItemInFocusChain(true).forceActiveFocus();
                } else {
                    incrementCurrentIndex()
                }
            }
            Keys.onBacktabPressed: {
                if (currentIndex == 0) {
                    listView.nextItemInFocusChain(false).forceActiveFocus();
                } else {
                    decrementCurrentIndex()
                }
            }
            Keys.onPressed: {
                if (event.text != "") {
                    queryField.text += event.text;
                    queryField.focus = true;
                }
            }
  
            Keys.onUpPressed: decrementCurrentIndex()
            Keys.onDownPressed: incrementCurrentIndex()

            function runCurrentIndex() {
                var entry = runnerWindow.history[currentIndex]
                if (entry) {
                    queryField.text = entry
                    queryField.forceActiveFocus();
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
