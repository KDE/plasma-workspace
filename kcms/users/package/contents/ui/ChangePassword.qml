/*
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.kcm.users 1.0 as UsersKCM

Kirigami.PromptDialog {
    id: passwordRoot

    property UsersKCM.User user

    title: i18n("Change Password")
    preferredWidth: Kirigami.Units.gridUnit * 20

    closePolicy: QQC2.Popup.CloseOnEscape
    showCloseButton: false
    standardButtons: QQC2.Dialog.Cancel

    customFooterActions: Kirigami.Action {
        id: passAction
        text: i18n("Set Password")
        enabled: !passwordWarning.visible && verifyField.text && passwordField.text
        onTriggered: apply()

        function apply() {
            if (passwordField.text !== verifyField.text) {
                debouncer.isTriggered = true
                return
            }
            passwordRoot.user.password = passwordField.text
            passwordRoot.close()
        }
    }

    onOpened: {
        passwordField.forceActiveFocus();
    }

    ColumnLayout {
        id: mainColumn
        spacing: Kirigami.Units.smallSpacing

        Kirigami.FormLayout {
            Kirigami.PasswordField {
                id: passwordField

                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("New password:")
                onTextChanged: debouncer.reset()

                onAccepted: {
                    passAction.trigger()
                }
            }

            Kirigami.PasswordField {
                id: verifyField

                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Confirm new password:")
                onTextChanged: debouncer.reset()

                onAccepted: {
                    passAction.trigger()
                }
            }
        }

        Debouncer {
            id: debouncer
        }

        Kirigami.InlineMessage {
            id: passwordWarning

            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            text: i18n("Passwords must match")
            visible: passwordField.text !== ""
                && verifyField.text !== ""
                && passwordField.text !== verifyField.text
                && debouncer.isTriggered
        }
    }
}
