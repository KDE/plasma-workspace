/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

import "../global"

RowLayout {
    id: replyRow

    // implicitWidth will keep being rewritten by the Layout itself
    Layout.maximumWidth: Globals.popupWidth
    required property ModelInterface modelInterface

    signal beginReplyRequested

    spacing: Kirigami.Units.smallSpacing

    function activate() {
        replyTextField.forceActiveFocus(Qt.ActiveWindowFocusReason);
    }

    Binding {
        target: replyRow.modelInterface
        property: "hasPendingReply"
        value: replyTextField.text !== ""
    }
    PlasmaComponents3.TextField {
        id: replyTextField
        Layout.fillWidth: true
        placeholderText: replyRow.modelInterface.replyPlaceholderText
                         || i18ndc("plasma_applet_org.kde.plasma.notifications", "Text field placeholder", "Type a reply…")
        Accessible.name: placeholderText
        onAccepted: {
            if (replyButton.enabled) {
                replyRow.modelInterface.replied(text);
            }
        }

        // Catches mouse click when reply field is already shown to start a reply
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.IBeamCursor
            visible: !replyRow.modelInterface.replying
            Accessible.name: "begin reply"
            Accessible.role: Accessible.Button
            Accessible.onPressAction: replyRow.beginReplyRequested()
            onClicked: mouse => {
                mouse.accepted = true
                replyRow.beginReplyRequested()
            }
        }
    }

    PlasmaComponents3.Button {
        id: replyButton
        icon.name: replyRow.modelInterface.replySubmitButtonIconName || "document-send"
        text: replyRow.modelInterface.replySubmitButtonText
              || i18ndc("plasma_applet_org.kde.plasma.notifications", "@action:button", "Send")
        enabled: replyTextField.length > 0
        onClicked: replyRow.modelInterface.replied(replyTextField.text)
    }
}
