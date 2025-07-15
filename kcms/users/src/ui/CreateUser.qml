/*
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5 as QQC2

import org.kde.kcmutils as KCM
import org.kde.kirigami 2.20 as Kirigami

KCM.SimpleKCM {
    title: i18n("Create User")

    onVisibleChanged: {
        realNameField.text = "";
        userNameField.text = "";
        passwordField.text = "";
        verifyField.text = "";
        usertypeBox.currentIndex = 0;
    }

    Component.onCompleted: {
        kcm.mainUi.createUserEnabled = false;
        realNameField.forceActiveFocus()
    }

    onBackRequested: {
        kcm.mainUi.createUserEnabled = true;
    }

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
                { type: "standard", label: i18n("Standard") },
                { type: "administrator", label: i18n("Administrator") },
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
            visible: passwordField.text !== ""
                && verifyField.text !== ""
                && passwordField.text !== verifyField.text
                && debouncer.isTriggered
        }
        Debouncer {
            id: debouncer
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true
            QQC2.Button {
                icon.name: "list-add"
                text: i18nc("@action:button Create a new user", "Create")
                enabled: !passwordWarning.visible
                    && realNameField.text !== ""
                    && userNameField.text !== ""
                    && passwordField.text !== ""
                    && verifyField.text !== ""

                onClicked: {
                    if (passwordField.text !== verifyField.text) {
                        debouncer.isTriggered = true;
                        return;
                    }
                    kcm.mainUi.createUser(userNameField.text, realNameField.text, passwordField.text, (usertypeBox.model[usertypeBox.currentIndex]["type"] === "administrator"));
                }
            }
            Item {
                Layout.fillWidth: true
            }
            QQC2.Button {
                // Only displayed in desktop mode
                icon.name: "dialog-cancel"
                text: i18nc("@action:button Cancel creating new user", "Cancel")
                visible: !Kirigami.Settings.isMobile

                onClicked: {
                    // Do not change page directly as it breaks navigation
                    kcm.mainUi.createUserEnabled = true;
                    kcm.mainUi.switchBackToUser();
                }
            }
        }
    }
}
