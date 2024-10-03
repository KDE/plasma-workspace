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

    property ModelInterface modelInterface

    property bool replying
    readonly property bool hasPendingReply: replyLoader.item?.text.length > 0

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
                var actionNames = (actionContainer.modelInterface.actionNames || []);
                var actionLabels = (actionContainer.modelInterface.actionLabels || []);

                for (var i = 0; i < actionNames.length; ++i) {
                    buttons.push({
                        actionName: actionNames[i],
                        label: actionLabels[i]
                    });
                }

                if (actionContainer.modelInterface.hasReplyAction) {
                    buttons.unshift({
                        actionName: "inline-reply",
                        label: actionContainer.modelInterface.replyActionLabel || i18nc("Reply to message", "Reply")
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

                    actionContainer.modelInterface.actionInvoked(modelData.actionName);
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
        active: actionContainer.replying || (actionContainer.modelInterface.hasReplyAction && (actionContainer.modelInterface.actionNames || []).length === 0)
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

            actionContainer.modelInterface.forceActiveFocusRequested();
            replyLoader.item.activate();
        }

        sourceComponent: NotificationReplyField {
            placeholderText: actionContainer.modelInterface.replyPlaceholderText
            buttonIconName: actionContainer.modelInterface.replySubmitButtonIconName
            buttonText: actionContainer.modelInterface.replySubmitButtonText
            onReplied: actionContainer.modelInterface.replied(text)

            replying: actionContainer.replying
            onBeginReplyRequested: replyLoader.beginReply()
        }
    }
}
