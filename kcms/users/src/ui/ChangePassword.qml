/*
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.kcm.users as UsersKCM

Kirigami.PromptDialog {
    id: passwordRoot

    preferredWidth: Kirigami.Units.gridUnit * 20

    iconName: 'lock-symbolic'

    function openAndClear() {
        verifyField.text = ""
        passwordField.text = ""
        passwordField.forceActiveFocus()
        this.open()
    }

    property UsersKCM.User user

    title: i18n("Change Password")

    standardButtons: Kirigami.Dialog.Cancel
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

    Label {
        text: i18nc("@label:textbox", "Password:")
        Layout.topMargin: Kirigami.Units.largeSpacing
        Layout.fillWidth: true
    }

    Kirigami.PasswordField {
        id: passwordField

        Layout.fillWidth: true

        onTextChanged: debouncer.reset()

        onAccepted: {
            if (!passwordWarning.visible && verifyField.text && passwordField.text) {
                passAction.trigger()
            }
        }
    }

    Label {
        text: i18nc("@label:textbox", "Confirm password:")
        Layout.topMargin: Kirigami.Units.largeSpacing
        Layout.fillWidth: true
    }

    Kirigami.PasswordField {
        id: verifyField

        Layout.fillWidth: true

        onTextChanged: debouncer.reset()

        onAccepted: {
            if (!passwordWarning.visible && verifyField.text && passwordField.text) {
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
