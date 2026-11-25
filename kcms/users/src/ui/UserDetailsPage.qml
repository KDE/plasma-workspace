/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.plasma.kcm.users as UsersKCM

KCM.SimpleKCM {
    id: usersDetailPage

    title: user.displayPrimaryName

    property UsersKCM.User user
    property bool overrideImage: false
    property url oldImage

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
            // Prompt to change the wallet password of the current user
            if (usersDetailPage.user.isCurrentUser && usersDetailPage.user.usesDefaultWallet()) {
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
            usersDetailPage.user.administrator = (usertypeBox.currentValue === "administrator")
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
        pending = pending || user.administrator != (usertypeBox.currentValue === "administrator")
        pending = pending || usersDetailPage.overrideImage
        return pending
    }

    function deleteUser(uid: int, deleteUserFiles: bool) {
        if (usersDetailPage.user.loggedIn) {
            deleteLoggedInUserWarningDialog.askedToDeleteUserFilesWhenImpossible = deleteUserFiles
            deleteLoggedInUserWarningDialog.open()
        } else {
            kcm.mainUi.deleteUser(usersDetailPage.user.uid, deleteUserFiles)
        }
    }

    Component.onCompleted: {
        kcm.needsSave = Qt.binding(resolvePending)
    }

    headerPaddingEnabled: false // Let the InlineMessages touch the edges
    header: Kirigami.InlineMessage {
        id: errorMessage
        visible: false
        type: Kirigami.MessageType.Error
        position: Kirigami.InlineMessage.Position.Header
    }

    Kirigami.PromptDialog {
        id: deleteLoggedInUserWarningDialog

        property bool askedToDeleteUserFilesWhenImpossible: false

        parent: usersDetailPage.QQC2.Overlay.overlay
        maximumWidth: Kirigami.Units.gridUnit * 30

        title: askedToDeleteUserFilesWhenImpossible
            ? i18nc("@title:window", "Delete Logged-In User Without Deleting Files?")
            : i18nc("@title:window", "Delete Logged-In User?")
        subtitle: askedToDeleteUserFilesWhenImpossible
            ? xi18nc("@info:usagetip", "%1 is currently logged in, so their files cannot be deleted. Delete just the account instead?<nl/><nl/>This will make %1 unable to log in again, and they may experience strange behaviors until they log out.", usersDetailPage.user.displayPrimaryName)
            : i18nc("@info:usagetip", "%1 is currently logged in. Deleting the account will make them unable to log in again, and they may experience strange behaviors until they log out.", usersDetailPage.user.displayPrimaryName)
        dialogType: Kirigami.PromptDialog.Warning
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

        onAccepted: kcm.mainUi.deleteUser(usersDetailPage.user.uid, false)

        Component.onCompleted: {
            standardButton(Kirigami.Dialog.Ok).text = i18nc("@action: button", "Delete %1", usersDetailPage.user.realName)
            standardButton(Kirigami.Dialog.Ok).icon.name = "edit-delete"
        }
    }

    Kirigami.Form {
        Kirigami.FormGroup {
            Kirigami.FormEntry {
                fullWidth: true

                // contentItem is wrapped to horizontally center the user avatar button
                contentItem: Item {
                    Layout.fillWidth: true
                    Kirigami.FormData.buddyFor: userAvatarButton

                    implicitHeight: userAvatarButton.height

                    QQC2.Button {
                        id: userAvatarButton
                        anchors.centerIn: parent

                        readonly property int size: 6 * Kirigami.Units.gridUnit
                        padding: Kirigami.Units.largeSpacing
                        implicitWidth: size + padding * 2
                        implicitHeight: size + padding * 2
                        Layout.alignment: Qt.AlignCenter
                        Layout.topMargin: Kirigami.Units.largeSpacing

                        focus: true
                        text: i18n("Change avatar")
                        QQC2.ToolTip.visible: text.length > 0 && (hovered || activeFocus)
                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

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

                        contentItem: Kirigami.Avatar {
                            source: usersDetailPage.user.faceValid ? usersDetailPage.user.face : ""
                            cache: false
                            name: user.realName

                            focus: true
                        }
                    }
                }
            }

            Kirigami.FormEntry {
                title: i18n("Username:")
                contentItem: QQC2.TextField {
                    id: userNameField
                    Layout.fillWidth: true
                    text: user.name
                    enabled: !usersDetailPage.user.loggedIn
                }

                trailingItems: Kirigami.ContextualHelpButton {
                    id: usernameContextButton
                    toolTipText: usersDetailPage.user.isCurrentUser
                    ? i18nc("@label:tooltip", "Cannot change your own username while logged in; log out and then log in as another user to change this account’s username.")
                    : i18nc("@label:tooltip", "Cannot change the username of a logged-in user; wait until this user logs out first.")
                    visible: !userNameField.enabled
                }
            }

            Kirigami.FormEntry {
                title: i18n("Name:")
                contentItem: QQC2.TextField  {
                    id: realNametextField
                    Layout.fillWidth: true
                    text: user.realName
                }
            }

            Kirigami.FormEntry {
                title: i18n("Email address:")
                contentItem: QQC2.TextField {
                    Layout.fillWidth: true
                    id: emailTextField
                    text: user.email
                }
            }
        }

        Kirigami.FormGroup {
            Kirigami.FormEntry {
                title: i18n("Account type:")
                contentItem: QQC2.ComboBox {
                    id: usertypeBox
                    enabled: kcm.userModel.moreThanOneAdminUser || !user.administrator

                    textRole: "label"
                    valueRole: "type"
                    model: [
                        { "type": "standard", "label": i18n("Standard") },
                        { "type": "administrator", "label": i18n("Administrator") },
                    ]

                    currentIndex: user.administrator ? 1 : 0
                }

                trailingItems: Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Cannot change the account type to Standard unless there is at least one other Administrator account on the system. Without one, authentication would become impossible or require the insecure use of the root password.")
                    visible: !usertypeBox.enabled
                }
                clickEnabled: usertypeBox.enabled
            }
        }


        Kirigami.FormGroup {
            Kirigami.FormAction {
                id: changePasswordAction
                fullWidth: true
                action: Kirigami.Action {
                    text: i18n("Change Password")
                    icon.name: "database-change-key" // TODO: We need a proper password icon
                    onTriggered: {
                        changePassword.user = user
                        changePassword.openAndClear()
                    }
                }
            }
            Kirigami.FormAction {
                id: fingerprintAction
                visible: true //kcm.fingerprintModel.deviceFound
                fullWidth: true
                action: Kirigami.Action {
                    id: fingerprintButton
                    text: i18n("Configure Fingerprint Authentication…")
                    icon.name: "fingerprint-gui"

                    property Kirigami.Dialog dialog: null

                    onTriggered: {
                        if (kcm.fingerprintModel.currentlyEnrolling) {
                            kcm.fingerprintModel.stopEnrolling();
                        }
                        kcm.fingerprintModel.switchUser(user.name == kcm.userModel.getCurrentUser().name ? "" : user.name);

                        if (fingerprintButton.dialog === null) {
                            const component = Qt.createComponent("FingerprintDialog.qml");
                            if (component.status == Component.Error) {
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
            }
            Kirigami.FormSeparator {}
            Kirigami.FormAction {
                id: deleteUser
                fullWidth: true
                action: Kirigami.Action {
                    id: deleteUserAction
                    enabled: !usersDetailPage.user.isCurrentUser
                    text: i18n("Delete User…")
                    icon.name: "edit-delete"
                    onTriggered: deleteMenu.open()
                }

                QQC2.Menu {
                    id: deleteMenu
                    modal: true
                    QQC2.MenuItem {
                        text: i18n("Delete files")
                        icon.name: "edit-delete-shred"
                        onClicked: {
                            usersDetailPage.deleteUser(usersDetailPage.user.uid, true);
                        }
                    }
                    QQC2.MenuItem {
                        text: i18n("Keep files")
                        icon.name: "document-multiple"
                        onClicked: {
                            usersDetailPage.deleteUser(usersDetailPage.user.uid, false);
                        }
                    }
                }
                trailingItems: Kirigami.ContextualHelpButton {
                    toolTipText: i18nc("@label:tooltip", "Cannot delete yourself! Log out and then log in as another user to delete this user account.")
                    visible: !deleteUserAction.enabled
                }
                clickEnabled: deleteUserAction.enabled
            }
        }
    }


    ChangePassword {
        id: changePassword
        usersDetailPage: usersDetailPage
    }

    ChangeWalletPassword {
        id: changeWalletPassword
        usersDetailPage: usersDetailPage
    }
}
