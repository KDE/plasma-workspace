/*
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5 as QQC2

import org.kde.kcm 1.2 as KCM
import org.kde.kirigami 2.20 as Kirigami

KCM.SimpleKCM {
    title: i18n("Create User")

    width: mainColumn.childrenRect.width + (Kirigami.Units.largeSpacing*10)
    height: mainColumn.childrenRect.height + (Kirigami.Units.largeSpacing*10)

    onVisibleChanged: {
        userNameField.text = realNameField.text = verifyField.text = passwordField.text = ""
        usertypeBox.currentIndex = 0
    }
    Component.onCompleted: realNameField.forceActiveFocus()
    Kirigami.FormLayout {
        anchors.centerIn: parent
        QQC2.TextField {
            id: realNameField
            Kirigami.FormData.label: i18n("Name:")
        }
        QQC2.TextField {
            id: userNameField
            Kirigami.FormData.label: i18n("Username:")
            validator: RegularExpressionValidator {
                regularExpression: /^[a-z_]([a-z0-9_-]{0,31}|[a-z0-9_-]{0,30}\$)$/
            }
        }
        QQC2.ComboBox {
            id: usertypeBox

            textRole: "label"
            model: [
                { "type": "standard", "label": i18n("Standard") },
                { "type": "administrator", "label": i18n("Administrator") },
            ]

            Kirigami.FormData.label: i18n("Account type:")
        }
        Kirigami.PasswordField {
            id: passwordField
            onTextChanged: debouncer.reset()
            Kirigami.FormData.label: i18n("Password:")
        }
        Kirigami.PasswordField {
            id: verifyField
            onTextChanged: debouncer.reset()
            Kirigami.FormData.label: i18n("Confirm password:")
        }
        Kirigami.InlineMessage {
            id: passwordWarning

            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            text: i18n("Passwords must match")
            visible: passwordField.text != "" && verifyField.text != "" && passwordField.text != verifyField.text && debouncer.isTriggered
        }
        Debouncer {
            id: debouncer
        }
        QQC2.Button {
            text: i18n("Create")
            enabled: !passwordWarning.visible && verifyField.text && passwordField.text && realNameField.text && userNameField.text
            onClicked: {
                if (passwordField.text != verifyField.text) {
                    debouncer.isTriggered = true
                    return
                }
                kcm.mainUi.createUser(userNameField.text, realNameField.text, passwordField.text, (usertypeBox.model[usertypeBox.currentIndex]["type"] == "administrator"))
            }
        }
    }
}
