/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components 1.0 as KirigamiComponents
import org.kde.kcmutils as KCM
import org.kde.plasma.kcm.users 1.0 as UsersKCM

KCM.SimpleKCM {
    id: usersDetailPage

    title: user.displayPrimaryName

    property UsersKCM.User user
    property bool overrideImage: false
    property url oldImage

    implicitWidth: Kirigami.Units.gridUnit * 30
    implicitHeight: Kirigami.Units.gridUnit * 27
    focus: true

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

        KirigamiComponents.AvatarButton {
            readonly property int size: 6 * Kirigami.Units.gridUnit

            Layout.preferredWidth: size
            Layout.preferredHeight: size
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.largeSpacing

            source: usersDetailPage.user.face
            cache: false
            name: user.realName

            focus: true
            text: i18n("Change avatar")

            KeyNavigation.down: realNametextField

            onClicked: {
                const component = Qt.createComponent("PicturesSheet.qml")
                const obj = component.incubateObject(usersDetailPage, {
                    focus: true,
                    usersDetailPage: usersDetailPage
                })
                if (obj == null) {
                    console.log(component.errorString())
                }
                component.destroy()
            }
        }

        Kirigami.FormLayout {
            QQC2.TextField  {
                id: realNametextField
                text: user.realName
                Kirigami.FormData.label: i18n("Name:")

                KeyNavigation.down: userNameField
            }

            QQC2.TextField {
                id: userNameField
                text: user.name
                Kirigami.FormData.label: i18n("Username:")

                KeyNavigation.down: usertypeBox
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

                KeyNavigation.down: emailTextField
            }

            QQC2.TextField {
                id: emailTextField
                text: user.email
                Kirigami.FormData.label: i18n("Email address:")
                KeyNavigation.down: changeButton
            }

            QQC2.Button {
                id: changeButton
                text: i18n("Change Password")
                KeyNavigation.down: deleteUser.enabled ? deleteUser : fingerprintButton
                onClicked: {
                    changePassword.user = user
                    changePassword.openAndClear()
                }
            }

            Item {
                Layout.preferredHeight: deleteUser.height
            }

            QQC2.Button {
                id: deleteUser

                enabled: !usersDetailPage.user.loggedIn && (!kcm.userModel.rowCount() < 2)

                KeyNavigation.down: fingerprintButton

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
            id: fingerprintButton
            Layout.topMargin: deleteUser.height
            Layout.alignment: Qt.AlignHCenter
            flat: false
            visible: kcm.fingerprintModel.deviceFound
            text: i18n("Configure Fingerprint Authentication…")
            icon.name: "fingerprint-gui"

            property Kirigami.Dialog dialog: null

            onClicked: {
                if (kcm.fingerprintModel.currentlyEnrolling) {
                    kcm.fingerprintModel.stopEnrolling();
                }
                kcm.fingerprintModel.switchUser(user.name == kcm.userModel.getLoggedInUser().name ? "" : user.name);

                if (fingerprintButton.dialog === null) {
                    const component = Qt.createComponent("FingerprintDialog.qml");

                    if (component.status === Component.Error) {
                        console.warn(component.errorString())
                    }

                    component.incubateObject(usersDetailPage, {
                        focus: true,
                    });
                    component.destroy();
                } else {
                    fingerprintButton.dialog.open()
                }
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

    ChangePassword { id: changePassword }
    ChangeWalletPassword { id: changeWalletPassword }
}
