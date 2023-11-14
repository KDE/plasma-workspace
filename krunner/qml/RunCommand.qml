/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1

import org.kde.config // KAuthorized
import org.kde.kcmutils // KCMLauncher
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.milou 0.1 as Milou
import org.kde.krunner.private.view
import org.kde.kirigami 2.20 as Kirigami

ColumnLayout {
    id: root
    property string query
    property string singleRunner
    property bool showHistory: false
    property string priorSearch: ""
    property alias runnerManager: results.runnerManager

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    // Spacing needs to be 0 to not make the last spacer item add a fake margin
    spacing: 0

    Connections {
        target: runnerWindow
        function onHistoryBehaviorChanged() {
            runnerManager.historyEnabled = runnerWindow.historyBehavior !== HistoryBehavior.Disabled
        }
        function onFavoriteIdsChanged() {
            results.model.favoriteIds = runnerWindow.favoriteIds
        }
    }
    Component.onCompleted: {
        runnerManager.historyEnabled = runnerWindow.historyBehavior !== HistoryBehavior.Disabled
        results.model.favoriteIds = runnerWindow.favoriteIds
    }

    onQueryChanged: {
        queryField.text = query;
    }

    Connections {
        target: runnerWindow
        function onVisibleChanged() {
            if (runnerWindow.visible) {
                queryField.forceActiveFocus();
                listView.currentIndex = -1
                if (runnerWindow.retainPriorSearch) {
                    // If we manually specified a query(D-Bus invocation) we don't want to retain the prior search
                    if (!query) {
                        queryField.text = priorSearch
                        queryField.select(root.query.length, 0)
                    }
                }
            } else {
                if (runnerWindow.retainPriorSearch) {
                    priorSearch = root.query
                }
                root.singleRunner = ""
                root.query = ""
                fadedTextCompletion.text = "";
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
                KCMLauncher.open("plasma/kcms/desktop/kcm_krunnersettings")
            }
            Accessible.name: i18n("Configure")
            Accessible.description: i18n("Configure KRunner Behavior")
            visible: KAuthorized.authorizeControlModule("kcm_krunnersettings")
            PlasmaComponents3.ToolTip {
                text: i18n("Configure KRunner…")
            }
        }
        PlasmaComponents3.ToolButton {
            visible: !!root.singleRunner
            icon.name: results.singleRunnerMetaData.iconName
            onClicked: () => {
                root.singleRunner = ""
                root.query = ""
                fadedTextCompletion.text = ""
                queryField.forceActiveFocus();
            }
            checked: true
            Accessible.description: i18n("Showing only results from %1", results.singleRunnerMetaData.name)
            PlasmaComponents3.ToolTip {
                text: parent.Accessible.description
            }
        }
        PlasmaExtras.SearchField {
            id: queryField
            property bool allowCompletion: false

            Layout.minimumWidth: Kirigami.Units.gridUnit * 25
            Layout.maximumWidth: Kirigami.Units.gridUnit * 25

            activeFocusOnPress: true
            placeholderText: results.singleRunner ? i18nc("Textfield placeholder text, query specific KRunner plugin",
                                                    "Search '%1'…", results.singleRunnerMetaData.name)
                                                : i18nc("Textfield placeholder text", "Search…")
            rightPadding: 0
            background.z: -2 // The fadedTextCompletion has -1, so it appears over the background

            QQC2.Label {
                id: fadedTextCompletion
                leftPadding: parent.leftPadding
                topPadding: parent.topPadding
                rightPadding: parent.rightPadding
                bottomPadding: parent.bottomPadding
                width: parent.width
                height: parent.height
                opacity: 0.5
                text: ""
                renderType: parent.renderType
                color: parent.color
                focus: false
                z: -1
            }

            PlasmaComponents3.BusyIndicator {
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    margins: Kirigami.Units.smallSpacing
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
                if (!allowCompletion) {
                    fadedTextCompletion.text = ""
                } else if (runnerWindow.historyBehavior === HistoryBehavior.CompletionSuggestion) {
                    fadedTextCompletion.text = runnerManager.getHistorySuggestion(text)
                } else if (length > 0 && runnerWindow.historyBehavior === HistoryBehavior.ImmediateCompletion) {
                    var oldText = text
                    var suggestedText = runnerManager.getHistorySuggestion(text);
                    if (suggestedText.length > 0) {
                        text = text + suggestedText.substr(oldText.length)
                        select(text.length, oldText.length)
                    }
                }
            }
            Keys.onPressed: event => {
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
                if (runnerWindow.historyBehavior === HistoryBehavior.CompletionSuggestion
                    && fadedTextCompletion.text.length > 0
                    && cursorPosition === text.length
                    && event.key === Qt.Key_Right
                ) {
                    queryField.text = fadedTextCompletion.text
                }
            }
            Keys.onTabPressed: event => {
                if (runnerWindow.historyBehavior === HistoryBehavior.CompletionSuggestion) {
                    if (fadedTextCompletion.text && queryField.text !== fadedTextCompletion.text) {
                        queryField.text = fadedTextCompletion.text
                    } else {
                        event.accepted = false
                    }
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
            Keys.onEnterPressed: event => closeOrRun(event)
            Keys.onReturnPressed: event => closeOrRun(event)

            Keys.onEscapePressed: {
                runnerWindow.visible = false
            }

            Kirigami.Icon {
                anchors {
                    right: parent.right
                    rightMargin: 6 // from PlasmaStyle TextFieldStyle
                    verticalCenter: parent.verticalCenter
                }
                // match clear button
                width: Math.max(parent.height * 0.8, Kirigami.Units.iconSizes.small)
                height: width
                source: "expand"
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
            visible: runnerWindow.helpEnabled
            checkable: true
            checked: root.query.startsWith("?")
            // Reset if out quers starts with "?", otherwise set it to "?"
            onClicked: root.query = root.query.startsWith("?") ? "" : "?"
            icon.name: "question"
            Accessible.name: i18n("Show Usage Help")
            Accessible.description: i18n("Show Usage Help")
            PlasmaComponents3.ToolTip {
                text: i18n("Show Usage Help")
            }
        }
        PlasmaComponents3.ToolButton {
            checkable: true
            checked: runnerWindow.pinned
            onToggled: runnerWindow.pinned = checked
            icon.name: "window-pin"
            Accessible.name: i18n("Pin")
            Accessible.description: i18n("Pin Search")
            PlasmaComponents3.ToolTip {
                text: i18n("Keep Open")
            }
        }
    }

    PlasmaComponents3.ScrollView {
        visible: results.count > 0
        enabled: visible
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.maximumHeight: results.contentHeight
        // This replaces the ColumnLayout spacing
        Layout.topMargin: Kirigami.Units.smallSpacing

        Milou.ResultsView {
            id: results
            queryString: root.query
            singleRunner: root.singleRunner

            Keys.onPressed: event => {
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
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.maximumHeight: results.contentHeight
        // This replaces the ColumnLayout spacing
        Layout.topMargin: Kirigami.Units.smallSpacing
        visible: root.query.length === 0 && listView.count > 0
        // don't accept keyboard input when not visible so the keys propagate to the other list
        enabled: visible

        ListView {
            id: listView // needs this id so the delegate can access it
            keyNavigationWraps: true
            highlight: PlasmaExtras.Highlight {}
            highlightMoveDuration: 0
            activeFocusOnTab: true
            model: []
            reuseItems: true
            delegate: Milou.ResultDelegate {
                id: resultDelegate
                width: listView.width
                typeText: index === 0 ? i18n("Recent Queries") : ""
                additionalActions: [{
                    icon: "list-remove",
                    text: i18n("Remove")
                }]
                Accessible.description: i18n("Recent Queries")
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
            Keys.onPressed: event => {
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

    // to align the layout to the top of any pending space in the view
    Item {
        Layout.fillHeight: true
    }
}
