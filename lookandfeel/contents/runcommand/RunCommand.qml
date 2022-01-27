/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents // For Highlight
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.milou 0.1 as Milou

ColumnLayout {
    id: root
    property string query
    property string runner
    property bool showHistory: false
    property alias runnerManager: results.runnerManager

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    onQueryChanged: {
        queryField.text = query;
    }

    Connections {
        target: runnerWindow
        function onVisibleChanged() {
            if (runnerWindow.visible) {
                queryField.forceActiveFocus();
                listView.currentIndex = -1
                if (runnerManager.retainPriorSearch) {
                    // If we manually specified a query(D-Bus invocation) we don't want to retain the prior search
                    if (!query) {
                        queryField.text = runnerManager.priorSearch
                        queryField.select(root.query.length, 0)
                    }
                }
            } else {
                if (runnerManager.retainPriorSearch) {
                    runnerManager.priorSearch = root.query
                }
                root.runner = ""
                root.query = ""
                root.showHistory = false
            }
        }
    }

    Connections {
        target: root
        function onShowHistoryChanged() {
            if (showHistory) {
                // we store 50 entries in the history but only show 20 in the UI so it doesn't get too huge
                listView.model = runnerManager.history.slice(0, 20)
            } else {
                listView.model = []
            }
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignTop
        PlasmaComponents3.ToolButton {
            icon.name: "configure"
            onClicked: {
                runnerWindow.visible = false
                runnerWindow.displayConfiguration()
            }
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Configure")
            Accessible.description: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Configure Search Plugins")
            visible: runnerWindow.canConfigure
            PlasmaComponents3.ToolTip {
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Configure KRunner…")
            }
        }
        PlasmaComponents3.TextField {
            id: queryField
            property bool allowCompletion: false

            clearButtonShown: true
            Layout.minimumWidth: PlasmaCore.Units.gridUnit * 25
            Layout.maximumWidth: PlasmaCore.Units.gridUnit * 25

            inputMethodHints: Qt.ImhNoPredictiveText

            activeFocusOnPress: true
            placeholderText: results.runnerName ? i18ndc("plasma_lookandfeel_org.kde.lookandfeel",
                                                         "Textfield placeholder text, query specific KRunner",
                                                         "Search '%1'…", results.runnerName)
                                                : i18ndc("plasma_lookandfeel_org.kde.lookandfeel",
                                                         "Textfield placeholder text", "Search…")

            PlasmaComponents3.BusyIndicator {
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    margins: PlasmaCore.Units.smallSpacing
                    rightMargin: height
                }

                Timer {
                    id: queryTimer
                    property bool queryDisplay: false
                    running: results.querying
                    repeat: true
                    onRunningChanged: if (queryDisplay && !running) {
                        queryDisplay = false
                    }
                    onTriggered: if (!queryDisplay) {
                        queryDisplay = true
                    }
                    interval: 500
                }

                running: queryTimer.queryDisplay
            }
            function move_up() {
                if (length === 0) {
                    root.showHistory = true;
                } else if (results.count > 0) {
                    results.decrementCurrentIndex();
                }
                focusCurrentListView()
            }

            function move_down() {
                if (length === 0) {
                    root.showHistory = true;
                } else if (results.count > 0) {
                    results.incrementCurrentIndex();
                }
                focusCurrentListView()
            }

            function focusCurrentListView() {
                if (listView.count > 0) {
                    listView.forceActiveFocus();
                } else if (results.count > 0) {
                    results.forceActiveFocus();
                }
            }

            onTextChanged: {
                root.query = queryField.text
                if (allowCompletion && length > 0 && runnerManager.historyEnabled) {
                    var oldText = text
                    var suggestedText = runnerManager.getHistorySuggestion(text);
                    if (suggestedText.length > 0) {
                        text = text + suggestedText.substr(oldText.length)
                        select(text.length, oldText.length)
                    }
                }
            }
            Keys.onPressed: {
                allowCompletion = (event.key !== Qt.Key_Backspace && event.key !== Qt.Key_Delete)

                if (event.modifiers & Qt.ControlModifier) {
                    if (event.key === Qt.Key_J) {
                        move_down()
                        event.accepted = true;
                    } else if (event.key === Qt.Key_K) {
                        move_up()
                        event.accepted = true;
                    }
                }
                // We only need to handle the Key_End case, the first item is focused by default
                if (event.key === Qt.Key_End && results.count > 0 && cursorPosition === text.length) {
                    results.currentIndex = results.count - 1
                    event.accepted = true;
                    focusCurrentListView()
                }
            }
            Keys.onUpPressed: move_up()
            Keys.onDownPressed: move_down()
            function closeOrRun(event) {
                // Close KRunner if no text was typed and enter was pressed, FEATURE: 211225
                if (!root.query) {
                    runnerWindow.visible = false
                } else {
                    results.runCurrentIndex(event)
                }
            }
            Keys.onEnterPressed: closeOrRun(event)
            Keys.onReturnPressed: closeOrRun(event)

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
                width: Math.max(parent.height * 0.8, PlasmaCore.Units.iconSizes.small)
                height: width
                svg: PlasmaCore.Svg {
                    imagePath: "widgets/arrows"
                    colorGroup: PlasmaCore.Theme.ButtonColorGroup
                }
                elementId: "down-arrow"
                visible: queryField.length === 0 && runnerManager.historyEnabled

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
        PlasmaComponents3.ToolButton {
            checkable: true
            checked: root.query.startsWith("?")
            // Reset if out quers starts with "?", otherwise set it to "?"
            onClicked: root.query = root.query.startsWith("?") ? "" : "?"
            icon.name: "question"
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Show Usage Help")
            Accessible.description: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Show Usage Help")
            PlasmaComponents3.ToolTip {
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Show Usage Help")
            }
        }
        PlasmaComponents3.ToolButton {
            checkable: true
            checked: runnerWindow.pinned
            onToggled: runnerWindow.pinned = checked
            icon.name: "window-pin"
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Pin")
            Accessible.description: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Pin Search")
            PlasmaComponents3.ToolTip {
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Keep Open")
            }
        }
    }

    PlasmaComponents3.ScrollView {
        Layout.alignment: Qt.AlignTop
        visible: results.count > 0
        enabled: visible
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(Screen.height, results.contentHeight)
        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        Milou.ResultsView {
            id: results
            queryString: root.query
            runner: root.runner

            Keys.onPressed: {
                var ctrl = event.modifiers & Qt.ControlModifier;
                if (ctrl && event.key === Qt.Key_J) {
                    incrementCurrentIndex()
                } else if (ctrl && event.key === Qt.Key_K) {
                    decrementCurrentIndex()
                } else if (event.key === Qt.Key_Home) {
                    results.currentIndex = 0
                } else if (event.key === Qt.Key_End) {
                    results.currentIndex = results.count - 1
                } else if (event.text !== "") {
                    // This prevents unprintable control characters from being inserted
                    if (!/[\x00-\x1F\x7F]/.test(event.text)) {
                        queryField.text += event.text;
                    }
                    queryField.cursorPosition = queryField.text.length
                    queryField.focus = true;
                }
            }

            Keys.onEscapePressed: {
                runnerWindow.visible = false
            }

            onActivated: {
                if (!runnerWindow.pinned) {
                    runnerWindow.visible = false
                }
            }

            onUpdateQueryString: {
                queryField.text = text
                queryField.select(cursorPosition, root.query.length)
                queryField.focus = true;
            }
        }
    }

    PlasmaComponents3.ScrollView {
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        visible: root.query.length === 0 && listView.count > 0
        // don't accept keyboard input when not visible so the keys propagate to the other list
        enabled: visible
        Layout.preferredHeight: Math.min(Screen.height, listView.contentHeight)
        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        ListView {
            id: listView // needs this id so the delegate can access it
            keyNavigationWraps: true
            highlight: PlasmaComponents.Highlight {}
            highlightMoveDuration: 0
            activeFocusOnTab: true
            model: []
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
            Keys.onReturnPressed: runCurrentIndex(event)
            Keys.onEnterPressed: runCurrentIndex(event)
            
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
                var ctrl = event.modifiers & Qt.ControlModifier;
                if (ctrl && event.key === Qt.Key_J) {
                    incrementCurrentIndex()
                } else if (ctrl && event.key === Qt.Key_K) {
                    decrementCurrentIndex()
                } else if (event.key === Qt.Key_Home) {
                    currentIndex = 0
                } else if (event.key === Qt.Key_End) {
                    currentIndex = count - 1
                } else if (event.text !== "") {
                    // This prevents unprintable control characters from being inserted
                    if (event.key == Qt.Key_Escape) {
                        root.showHistory = false
                    } else if (!/[\x00-\x1F\x7F]/.test(event.text)) {
                        queryField.text += event.text;
                    }
                    queryField.focus = true;
                }
            }
  
            Keys.onUpPressed: decrementCurrentIndex()
            Keys.onDownPressed: incrementCurrentIndex()

            function runCurrentIndex(event) {
                var entry = runnerManager.history[currentIndex]
                if (entry) {
                    // If user presses Shift+Return to invoke an action, invoke the first runner action
                    if (event && event.modifiers === Qt.ShiftModifier
                            && currentItem.additionalActions && currentItem.additionalActions.length > 0) {
                        runAction(0);
                        return
                    }

                    queryField.text = entry
                    queryField.forceActiveFocus();
                }
            }

            function runAction(actionIndex) {
                if (actionIndex === 0) {
                    // QStringList changes just reset the model, so we'll remember the index and set it again
                    var currentIndex = listView.currentIndex
                    runnerManager.removeFromHistory(currentIndex)
                    model = runnerManager.history
                    listView.currentIndex = currentIndex
                }
            }
        }

    }
}
