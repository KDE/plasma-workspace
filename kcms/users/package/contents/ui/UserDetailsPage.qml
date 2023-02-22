/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.12
import QtQuick.Controls 2.5 as QQC2

import org.kde.kcm 1.2
import org.kde.kirigami 2.13 as Kirigami

SimpleKCM {
    id: usersDetailPage

    title: user.displayPrimaryName

    property QtObject user
    property bool overrideImage: false
    property url oldImage

    Connections {
        target: user
        function onApplyError(errorText) {
            errorMessage.visible = true
            errorMessage.text = errorText
        }
    }

    Connections {
        target: user
        function onPasswordSuccessfullyChanged() {
            // Prompt to change the wallet password of the logged-in user
            if (usersDetailPage.user.loggedIn && usersDetailPage.user.usesDefaultWallet()) {
                changeWalletPassword.open()
            }
        }
    }


    Connections {
        target: kcm

        function onApply() {
            errorMessage.visible = false
            usersDetailPage.user.realName = realNametextField.text
            usersDetailPage.user.email = emailTextField.text
            usersDetailPage.user.name = userNameField.text
            usersDetailPage.user.administrator = (usertypeBox.model[usertypeBox.currentIndex]["type"] == "administrator")
            user.apply()
            usersDetailPage.overrideImage = false
            usersDetailPage.oldImage = ""
        }

        function onReset() {
            errorMessage.visible = false
            realNametextField.text = usersDetailPage.user.realName
            emailTextField.text = usersDetailPage.user.email
            userNameField.text = usersDetailPage.user.name
            usertypeBox.currentIndex = usersDetailPage.user.administrator ? 1 : 0
            if (usersDetailPage.oldImage != "") {
                usersDetailPage.overrideImage = false
                usersDetailPage.user.face = usersDetailPage.oldImage
            }
        }
    }

    function resolvePending() {
        let pending = false
        let user = usersDetailPage.user
        pending = pending || user.realName != realNametextField.text
        pending = pending || user.email != emailTextField.text
        pending = pending || user.name != userNameField.text
        pending = pending || user.administrator != (usertypeBox.model[usertypeBox.currentIndex]["type"] == "administrator")
        pending = pending || usersDetailPage.overrideImage
        return pending
    }

    Component.onCompleted: {
        kcm.needsSave = Qt.binding(resolvePending)
    }

    ColumnLayout {
        Kirigami.InlineMessage {
            id: errorMessage
            visible: false
            type: Kirigami.MessageType.Error
            Layout.fillWidth: true
        }

        Kirigami.Avatar {
            readonly property int size: 6 * Kirigami.Units.gridUnit
            Layout.preferredWidth: size
            Layout.preferredHeight: size
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.largeSpacing

            source: usersDetailPage.user.face
            cache: false
            name: user.realName

            actions {
                main: Kirigami.Action {
                    text: i18n("Change avatar")
                    onTriggered: {
                        const component = Qt.createComponent("PicturesSheet.qml")
                        component.incubateObject(usersDetailPage, {
                            "parent": usersDetailPage,
                        })
                        component.destroy()
                    }
                }
            }
        }

        Kirigami.FormLayout {
            QQC2.TextField  {
                id: realNametextField
                focus: true
                text: user.realName
                Kirigami.FormData.label: i18n("Name:")
            }

            QQC2.TextField {
                id: userNameField
                focus: true
                text: user.name
                Kirigami.FormData.label: i18n("Username:")
            }

            QQC2.ComboBox {
                id: usertypeBox

                textRole: "label"
                model: [
                    { "type": "standard", "label": i18n("Standard") },
                    { "type": "administrator", "label": i18n("Administrator") },
                ]

                Kirigami.FormData.label: i18n("Account type:")

                currentIndex: user.administrator ? 1 : 0
            }

            QQC2.TextField {
                id: emailTextField
                focus: true
                text: user.email
                Kirigami.FormData.label: i18n("Email address:")
            }

            QQC2.Button {
                text: i18n("Change Password")
                onClicked: {
                    changePassword.account = user
                    changePassword.openAndClear()
                }
            }

            Item {
                Layout.preferredHeight: deleteUser.height
            }

            QQC2.Button {
                id: deleteUser

                enabled: !usersDetailPage.user.loggedIn && (!kcm.userModel.rowCount() < 2)

                QQC2.Menu {
                    id: deleteMenu
                    modal: true
                    QQC2.MenuItem {
                        text: i18n("Delete files")
                        icon.name: "edit-delete-shred"
                        onClicked: {
                            kcm.mainUi.deleteUser(usersDetailPage.user.uid, true)
                        }
                    }
                    QQC2.MenuItem {
                        text: i18n("Keep files")
                        icon.name: "document-multiple"
                        onClicked: {
                            kcm.mainUi.deleteUser(usersDetailPage.user.uid, false)
                        }
                    }
                }
                text: i18n("Delete User…")
                icon.name: "edit-delete"
                onClicked: deleteMenu.open()
            }
        }

        QQC2.Button {
            Layout.topMargin: deleteUser.height
            Layout.alignment: Qt.AlignHCenter
            flat: false
            visible: kcm.fingerprintModel.deviceFound
            text: i18n("Configure Fingerprint Authentication…")
            icon.name: "fingerprint-gui"
            onClicked: {
                fingerprintDialog.account = user;
                fingerprintDialog.openAndClear();
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing * 2
            Layout.rightMargin: Kirigami.Units.largeSpacing * 2

            visible: kcm.fingerprintModel.deviceFound

            text: xi18nc("@info", "Fingerprints can be used in place of a password when unlocking the screen and providing administrator permissions to applications and command-line programs that request them.<nl/><nl/>Logging into the system with your fingerprint is not yet supported.")

            font: Kirigami.Theme.smallFont
            wrapMode: Text.Wrap
        }
    }

    ChangePassword { id: changePassword; account: user }
    ChangeWalletPassword { id: changeWalletPassword }
    FingerprintDialog { id: fingerprintDialog; account: user }
}
