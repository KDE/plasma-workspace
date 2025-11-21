/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T

import org.kde.kirigami as Kirigami
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.private.clipboard 0.1 as Private

Loader {
    id: actionMenu

    required property string uuid
    required property string text
    required property bool automaticallyInvoked
    readonly property var matchingActions: Private.URLGrabber.matchingActions(text, automaticallyInvoked)
    property int hoveredCount: 0

    signal requestHidePopup
    signal requestResizeHeight(real contentHeight)

    property PlasmaExtras.PlasmoidHeading header: PlasmaExtras.PlasmoidHeading {
        PlasmaComponents3.Button {
            anchors.fill: parent
            icon.name: "go-previous-view"
            text: actionMenu.automaticallyInvoked ?
                i18ndc("klipper", "@action:button", "Cancel") :
                i18nd("klipper", "Return to Clipboard")
            onClicked: actionMenu.exit(actionMenu.automaticallyInvoked)
        }
    }

    sourceComponent: matchingActions.length > 0 ? menuComponent : placeholderMessage

    function exit(hidePopup: bool) {
        cancelTimer.running = false;
        if (hidePopup) {
            actionMenu.requestHidePopup();
        }
        T.StackView.view.popCurrentItem();
    }

    Timer {
        id: cancelTimer
        interval: 1000 * Private.URLGrabber.popupKillTimeout
        repeat: false
        running: interval >= 1000
            && actionMenu.automaticallyInvoked
            && actionMenu.T.StackView.visible
            && actionMenu.hoveredCount === 0
        onTriggered: actionMenu.exit(true)
    }

    Component {
        id: placeholderMessage
        Item {
            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.gridUnit * 4)
                iconName: "dialog-cancel-symbolic"
                text: i18nd("klipper", "No actions found for this text.")
            }
        }
    }

    Component {
        id: menuComponent
        PlasmaComponents3.ScrollView {
            id: scrollView
            background: null
            contentWidth: availableWidth - (contentItem as ListView).leftMargin - (contentItem as ListView).rightMargin
            focus: true
            Keys.onEscapePressed: actionMenu.exit(actionMenu.automaticallyInvoked)
            PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff
            onImplicitHeightChanged: if (height < implicitHeight) {
                actionMenu.requestResizeHeight(implicitHeight)
            }
            contentItem: ListView {
                id: listView
                currentIndex: 0
                focus: true
                reuseItems: true

                topMargin: 0
                bottomMargin: 0
                leftMargin: Kirigami.Units.largeSpacing
                rightMargin: Kirigami.Units.largeSpacing
                spacing: Kirigami.Units.smallSpacing

                highlightFollowsCurrentItem: true
                highlightMoveDuration: 0
                highlightResizeDuration: 0
                highlight: PlasmaExtras.Highlight { }

                model: actionMenu.matchingActions
                delegate: PlasmaComponents3.ItemDelegate {
                    required property int index
                    required property string actionText
                    required property string iconText
                    required property var command
                    required property list<string> actionCapturedTexts
                    width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
                    text: actionText
                    icon.name: iconText
                    hoverEnabled: true
                    onClicked: {
                        Private.URLGrabber.execute(actionMenu.uuid, actionMenu.text, command, actionCapturedTexts);
                        actionMenu.exit(true);
                    }
                    onHoveredChanged: {
                        actionMenu.hoveredCount += hovered ? 1 : -1
                        if (hovered) {
                            ListView.view.currentIndex = index;
                        }
                    }
                    Keys.onEnterPressed: event => Keys.returnPressed(event)
                }
                section {
                    property: "sectionName"
                    delegate: PlasmaExtras.ListSectionHeader {
                        required property string section
                        width: listView.width - listView.leftMargin - listView.rightMargin
                        text: section
                    }
                }
            }
        }
    }
}
