/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

T.StackView { // FIXME: put a StackView in PlasmaComponents3
    id: actionContainer

    property ModelInterface modelInterface

    property bool replying
    readonly property bool hasPendingReply: depth > 1 && currentItem?.text?.length > 0

    implicitWidth: currentItem.implicitWidth
    implicitHeight: currentItem.implicitHeight

    function beginReply() {
        replying = true;
        modelInterface.forceActiveFocusRequested();
        if (currentItem === actionRow) {
            push(replyFieldComponent);
            currentItem.activate()
        }
    }

    pushEnter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Kirigami.Units.longDuration
            easing.type: Easing.InQuad
        }
    }
    pushExit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutQuad
        }
    }

    // Notification actions
    initialItem: RowLayout {
        id: actionRow
        spacing: 0

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
                        actionContainer.beginReply();
                        return;
                    }

                    actionContainer.modelInterface.actionInvoked(modelData.actionName);
                }
            }
        }
    }

    Component {
        id: replyFieldComponent
        NotificationReplyField {
            placeholderText: actionContainer.modelInterface.replyPlaceholderText
            buttonIconName: actionContainer.modelInterface.replySubmitButtonIconName
            buttonText: actionContainer.modelInterface.replySubmitButtonText
            onReplied: actionContainer.modelInterface.replied(text)

            replying: actionContainer.replying
            onBeginReplyRequested: actionContainer.beginReply()
        }
    }

    Component.onCompleted: {
        if (actionContainer.modelInterface.hasReplyAction && (actionContainer.modelInterface.actionNames || []).length === 0) {
            actionContainer.beginReply();
        }
    }
}
