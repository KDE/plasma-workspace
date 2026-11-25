/*
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: createUserDialog
    title: i18n("Create User")
    visible: true
    padding: Kirigami.Units.largeSpacing

    onVisibleChanged: {
        realNameField.text = "";
        userNameField.text = "";
        userNameField.hadUppercase = false;
        userNameField.hadInvalidChars = false;
        passwordField.text = "";
        verifyField.text = "";
        usertypeBox.currentIndex = 0;
    }

    Component.onCompleted: {
        realNameField.forceActiveFocus();
    }

    Kirigami.FormLayout {
        anchors.centerIn: parent

        QQC2.TextField {
            Kirigami.FormData.label: i18n("Name:")
            id: realNameField
        }

        QQC2.TextField {
            id: userNameField
            Kirigami.FormData.label: i18n("Username:")

            property bool hadUppercase: false
            property bool hadInvalidChars: false
            readonly property bool isUsernameValid: /^[a-z_]([a-z0-9_-]{0,31}|[a-z0-9_-]{0,30}\$)$/.test(text)

            onTextEdited: {
                if (text === "") {
                    hadUppercase = false;
                    hadInvalidChars = false;
                    return;
                }

                const pos = cursorPosition;
                const original = text;
                const lower = original.toLowerCase();
                const cleaned = lower.replace(/[^a-z0-9_$-]/g, "");

                if (original !== cleaned) {
                    hadUppercase = original !== lower;
                    hadInvalidChars = lower !== cleaned;
                    text = cleaned;
                    cursorPosition = Math.min(pos, text.length);
                } else {
                    hadUppercase = false;
                    hadInvalidChars = false;
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Warning
            showCloseButton: false
            visible: userNameField.text !== "" && (userNameField.hadUppercase || userNameField.hadInvalidChars)
            text: {
                if (userNameField.hadInvalidChars) {
                    return i18nc("@info:usagetip", "Only lowercase letters, digits, dashes, and underscores are allowed. Invalid characters were removed.");
                }
                return i18nc("@info:usagetip", "Usernames must be lowercase. Uppercase characters were converted to lowercase.");
            }
        }

        Kirigami.PasswordField {
            Kirigami.FormData.label: i18n("Password:")
            id: passwordField
            onTextChanged: debouncer.reset()
        }

        Kirigami.PasswordField {
            Kirigami.FormData.label: i18n("Confirm password:")
            id: verifyField
            onTextChanged: debouncer.reset()
        }

        Kirigami.InlineMessage {
            id: passwordWarning
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            text: i18n("Passwords must match")
            visible: passwordField.text !== "" && verifyField.text !== "" && passwordField.text !== verifyField.text && debouncer.isTriggered
        }

        Debouncer {
            id: debouncer
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Account type:")
            id: usertypeBox
            Layout.fillWidth: true

            textRole: "label"
            model: [
                {
                    type: "standard",
                    label: i18n("Standard")
                },
                {
                    type: "administrator",
                    label: i18n("Administrator")
                },
            ]
        }
    }

    customFooterActions: [
        Kirigami.Action {
            icon.name: "list-add"
            text: i18nc("@action:button Create a new user", "Create")
            enabled: !passwordWarning.visible && realNameField.text !== "" && userNameField.isUsernameValid && passwordField.text !== "" && verifyField.text !== ""

            onTriggered: {
                if (passwordField.text !== verifyField.text) {
                    debouncer.isTriggered = true;
                    return;
                }
                const userTypeIsAdmin = (usertypeBox.model[usertypeBox.currentIndex]["type"] === "administrator");
                const userIsCreated = kcm.mainUi.createUser(userNameField.text, realNameField.text, passwordField.text, userTypeIsAdmin);
                createUserDialog.close()
            }
        },
        Kirigami.Action {
            // Only displayed in desktop mode
            icon.name: "dialog-cancel"
            text: i18nc("@action:button Cancel creating new user", "Cancel")
            visible: !Kirigami.Settings.isMobile

            onTriggered: {
                createUserDialog.close()
            }
        }
    ]
}
