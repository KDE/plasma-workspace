/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window

import org.kde.config // KAuthorized
import org.kde.kcmutils // KCMLauncher
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.milou as Milou
import org.kde.krunner.private.view
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root

    required property RunnerWindow runnerWindow
    property string query
    property string singleRunner
    property bool showHistory: false
    property string priorSearch: ""
    property alias runnerManager: results.runnerManager

    LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    function isKeyUp(event) {
        return event.key === Qt.Key_Up || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_K)
    }
    function isKeyDown(event) {
        return event.key === Qt.Key_Down || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_J)
    }

    // Spacing needs to be 0 to not make the last spacer item add a fake margin
    spacing: 0

    Connections {
        target: root.runnerWindow
        function onHistoryBehaviorChanged() {
            root.runnerManager.historyEnabled = root.runnerWindow.historyBehavior !== RunnerWindow.Disabled
        }
        function onActivityChanged(activity) {
            if (activity) {
                root.runnerManager.setHistoryEnvironmentIdentifier(activity)
            }
        }
    }
    Component.onCompleted: {
        runnerManager.historyEnabled = runnerWindow.historyBehavior !== RunnerWindow.Disabled
    }

    onQueryChanged: {
        queryField.text = query;
    }

    Connections {
        target: root.runnerWindow
        function onVisibleChanged() {
            if (root.runnerWindow.visible) {
                queryField.forceActiveFocus();
                listView.currentIndex = -1
                if (root.runnerWindow.retainPriorSearch) {
                    // If we manually specified a query(D-Bus invocation) we don't want to retain the prior search
                    if (!root.query) {
                        queryField.text = root.priorSearch
                        queryField.select(root.query.length, 0)
                        fadedTextCompletion.text = ""
                    }
                }
            } else {
                if (root.runnerWindow.retainPriorSearch) {
                    root.priorSearch = root.query
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
            if (root.showHistory) {
                // we store 50 entries in the history but only show 20 in the UI so it doesn't get too huge
                listView.model = root.runnerManager.history.slice(0, 20)
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
                root.runnerWindow.visible = false
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
            background.z: -2 // The fadedTextCompletion has -1, so it appears over the background

            QQC2.Label {
                id: fadedTextCompletion
                leftPadding: parent.leftPadding
                topPadding: parent.topPadding
                rightPadding: parent.rightPadding
                bottomPadding: parent.bottomPadding
                width: parent.width
                height: parent.height
                elide: Text.ElideRight
                visible: text !== queryField.text
                    && queryField.implicitWidth < (queryField.width
                                                - queryField.leftPadding
                                                - queryField.rightPadding)
                opacity: 0.5
                text: ""
                textFormat: Text.PlainText
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

            function focusCurrentListView() {
                if (listView.count > 0) {
                    listView.forceActiveFocus();
                } else if (results.count > 0) {
                    results.forceActiveFocus();
                }
            }

            onTextChanged: {
                root.query = queryField.text
                if (!allowCompletion || !root.query ) { // Clear suggestion in case it was disabled or the query is cleared
                    fadedTextCompletion.text = ""
                } else if (root.runnerWindow.historyBehavior === RunnerWindow.CompletionSuggestion) {
                    // Match the user's exact typed characters to account for case insensitive matches
                    fadedTextCompletion.text = text + root.runnerManager.getHistorySuggestion(text).substring(text.length)
                } else if (length > 0 && root.runnerWindow.historyBehavior === RunnerWindow.ImmediateCompletion) {
                    var oldText = text
                    var suggestedText = root.runnerManager.getHistorySuggestion(text);
                    if (suggestedText.length > 0) {
                        text = text + suggestedText.substr(oldText.length)
                        select(text.length, oldText.length)
                    }
                }
            }
            Keys.onPressed: event => {
                allowCompletion = (event.key !== Qt.Key_Backspace && event.key !== Qt.Key_Delete)
                // We only need to handle the Key_End case, the first item is focused by default
                if (event.key === Qt.Key_End && results.count > 0 && cursorPosition === text.length) {
                    results.currentIndex = results.count - 1
                    event.accepted = true;
                    focusCurrentListView()
                }
                if (root.runnerWindow.historyBehavior === RunnerWindow.CompletionSuggestion
                    && fadedTextCompletion.text.length > 0
                    && cursorPosition === text.length
                    && event.key === Qt.Key_Right
                ) {
                    queryField.text = fadedTextCompletion.text
                    fadedTextCompletion.text = ""
                    event.accepted = true
                }
                if (queryField.text.length === 0 && (root.isKeyUp(event) || root.isKeyDown(event))) {
                    event.accepted = true
                    root.showHistory = true;
                    focusCurrentListView()
                }
                !event.accepted && results.navigationKeyHandler(event)
            }
            Keys.onTabPressed: event => {
                if (root.runnerWindow.historyBehavior === RunnerWindow.CompletionSuggestion) {
                    if (fadedTextCompletion.text && queryField.text !== fadedTextCompletion.text) {
                        queryField.text = fadedTextCompletion.text
                        fadedTextCompletion.text = ""
                    } else {
                        event.accepted = false
                    }
                }
            }
            function closeOrRun(event) {
                // Close KRunner if no text was typed and enter was pressed, FEATURE: 211225
                if (!root.query) {
                    root.runnerWindow.visible = false
                } else {
                    results.runCurrentIndex(event)
                }
            }
            Keys.onEnterPressed: event => closeOrRun(event)
            Keys.onReturnPressed: event => closeOrRun(event)

            Keys.onEscapePressed: {
                root.runnerWindow.visible = false
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
                visible: queryField.length === 0 && root.runnerManager.historyEnabled
                enabled: root.runnerManager.history.length > 0

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
            visible: root.runnerWindow.helpEnabled
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
            checked: root.runnerWindow.pinned
            onToggled: root.runnerWindow.pinned = checked
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
        Layout.maximumHeight: Math.max(listView.contentHeight, results.contentHeight)
        // This replaces the ColumnLayout spacing
        Layout.topMargin: Kirigami.Units.smallSpacing

        Milou.ResultsView {
            id: results
            queryString: root.query
            queryField: queryField
            singleRunner: root.singleRunner

            Keys.onEscapePressed: {
                root.runnerWindow.visible = false
            }

            onActivated: {
                if (!root.runnerWindow.pinned) {
                    root.runnerWindow.visible = false
                }
            }

            onUpdateQueryString: (text, cursorPosition) => {
                queryField.text = text
                queryField.select(cursorPosition, root.query.length)
                queryField.focus = true;
            }
        }
    }

    PlasmaComponents3.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.maximumHeight: listView.contentHeight
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

                required property var modelData

                width: listView.width
                typeText: index === 0 ? i18n("Recent Queries") : ""
                displayText: modelData
                actions: [{
                    iconSource: "list-remove-symbolic",
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
                if (root.isKeyUp(event)) {
                    decrementCurrentIndex();
                } else if (root.isKeyDown(event)) {
                    incrementCurrentIndex();
                } else if (event.text !== "" && !event.accepted) {
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
                var entry = root.runnerManager.history[currentIndex]
                if (entry) {
                    // If user presses Shift+Return to invoke an action, invoke the first runner action
                    if (event && event.modifiers === Qt.ShiftModifier
                            && currentItem.actions && currentItem.actions.length > 0) {
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
                    root.runnerManager.removeFromHistory(currentIndex)
                    model = root.runnerManager.history
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
