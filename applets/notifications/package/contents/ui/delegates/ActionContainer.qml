/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

StackLayout { // TODO: StackView?
    id: actionContainer

    property var actionNames: []
    property var actionLabels: []

    property bool hasReplyAction
    property bool replying
    readonly property bool hasPendingReply: replyLoader.item?.text.length > 0
    property string replyActionLabel
    property string replyPlaceholderText
    property string replySubmitButtonIconName
    property string replySubmitButtonText

    signal forceActiveFocusRequested()
    signal actionInvoked(string actionName)
    signal replied(string text)

    Layout.fillWidth: true
    visible: actionRepeater.count > 0

    currentIndex: replyLoader.active ? 1 : 0

    // Notification actions
    RowLayout {
        id: actionRow
        // For a cleaner look, if there is a thumbnail, puts the actions next to the thumbnail strip's menu button
        //FIXME parent: thumbnailStripLoader.item?.actionContainer ?? actionContainer
        width: parent.width
        spacing: 0

        enabled: !replyLoader.active
        opacity: replyLoader.active ? 0 : 1
        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Repeater {
            id: actionRepeater

            model: {
                var buttons = [];
                var actionNames = (actionContainer.actionNames || []);
                var actionLabels = (actionContainer.actionLabels || []);

                for (var i = 0; i < actionNames.length; ++i) {
                    buttons.push({
                        actionName: actionNames[i],
                        label: actionLabels[i]
                    });
                }

                if (actionContainer.hasReplyAction) {
                    buttons.unshift({
                        actionName: "inline-reply",
                        label: actionContainer.replyActionLabel || i18nc("Reply to message", "Reply")
                    });
                }

                return buttons;
            }

            PlasmaComponents3.ToolButton {
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth
                Layout.leftMargin: index > 0 ? Kirigami.Units.smallSpacing : 0

                flat: false
                // why does it spit "cannot assign undefined to string" when a notification becomes expired?
                text: modelData.label || ""

                onClicked: {
                    if (modelData.actionName === "inline-reply") {
                        replyLoader.beginReply();
                        return;
                    }

                    actionContainer.actionInvoked(modelData.actionName);
                }
            }
        }
    }

    // inline reply field
    Loader {
        id: replyLoader
        width: parent.width
        height: active && item ? item.implicitHeight : 0
        // When there is only one action and it is a reply action, show text field right away
        active: actionContainer.replying || (actionContainer.hasReplyAction && (actionContainer.actionNames || []).length === 0)
        visible: active
        opacity: active ? 1 : 0
        x: active ? 0 : parent.width
        Behavior on x {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }

        function beginReply() {
            actionContainer.replying = true;

            actionContainer.forceActiveFocusRequested();
            replyLoader.item.activate();
        }

        sourceComponent: NotificationReplyField {
            placeholderText: actionContainer.replyPlaceholderText
            buttonIconName: actionContainer.replySubmitButtonIconName
            buttonText: actionContainer.replySubmitButtonText
            onReplied: actionContainer.replied(text)

            replying: actionContainer.replying
            onBeginReplyRequested: replyLoader.beginReply()
        }
    }
}
