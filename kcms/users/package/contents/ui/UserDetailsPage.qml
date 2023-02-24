/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kcm 1.2 as KCM
import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.kcm.users 1.0 as UsersKCM

KCM.SimpleKCM {
    id: usersDetailPage

    title: user.displayPrimaryName

    property UsersKCM.User user
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
                const dialog = changeWalletPasswordDialogComponent.createObject(usersDetailPage, {
                    user: usersDetailPage.user,
                });
                dialog.open();
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

    function showDeleteUserOptionsDialog() {
        const dialog = deleteUserOptionsDialogComponent.createObject(this);
        dialog.open();
    }

    function showDeleteUserConfirmationDialog(alsoDeleteFiles: bool) {
        // Somehow applicationWindow() magic does not work in this context.
        let root = this;
        while (typeof root.applicationWindow === "undefined" && root.parent !== null) {
            root = root.parent;
        }
        const dialog = deleteUserConfirmationDialogComponent.createObject(this, {
            alsoDeleteFiles,
            user,
            windowContentItem: root,
        });
        dialog.userDeleted.connect((successfully, userName) => {
            showDeleteUserResultDialog(successfully, userName);
        });
        dialog.open();
    }

    function showDeleteUserResultDialog(success: bool, userName: string) {
        const dialog = deleteUserResultDialogComponent.createObject(this, {
            success,
            userName,
        });
        dialog.open();
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
                            parent: usersDetailPage,
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
                    const dialog = changePasswordDialogComponent.createObject(usersDetailPage, {
                        user: usersDetailPage.user,
                    });
                    dialog.open();
                }
            }

            Item {
                Layout.preferredHeight: deleteUserButton.height
            }

            QQC2.Button {
                id: deleteUserButton

                enabled: !usersDetailPage.user.loggedIn && (!kcm.userModel.rowCount() < 2)
                text: i18n("Delete User…")
                icon.name: "edit-delete"
                onClicked: {
                    usersDetailPage.showDeleteUserOptionsDialog();
                }
            }
        }

        QQC2.Button {
            Layout.topMargin: deleteUserButton.height
            Layout.alignment: Qt.AlignHCenter
            flat: false
            visible: kcm.fingerprintModel.deviceFound
            text: i18n("Configure Fingerprint Authentication…")
            icon.name: "fingerprint-gui"
            onClicked: {
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

    ChangePassword { id: changePassword }

    Component {
        id: changePasswordDialogComponent
        ChangePassword {
            parent: usersDetailPage.Kirigami.ColumnView.view
            onClosed: {
                destroy();
            }
        }
    }

    Component {
        id: changeWalletPasswordDialogComponent
        ChangeWalletPassword {
            parent: usersDetailPage.Kirigami.ColumnView.view
            onClosed: {
                destroy();
            }
        }
    }

    FingerprintDialog {
        id: fingerprintDialog
        user: usersDetailPage.user
    }

    Component {
        id: deleteUserOptionsDialogComponent
        Kirigami.PromptDialog {
            id: dialog

            // This is more like a single tristate property, or Option<bool>:
            // `alsoDeleteFiles` is meaningless while `checkedButton` is null
            readonly property bool canProceed: deleteFilesButtonGroup.checkedButton !== null
            readonly property bool alsoDeleteFiles: deleteFilesButtonGroup.checkedButton === deleteFilesRadioButton

            parent: usersDetailPage.Kirigami.ColumnView.view
            title: i18n("Delete User?")
            subtitle: i18n("This operation cannot be undone")

            closePolicy: QQC2.Popup.CloseOnEscape
            showCloseButton: true
            standardButtons: QQC2.Dialog.Cancel

            onAccepted: {
                usersDetailPage.showDeleteUserConfirmationDialog(dialog.alsoDeleteFiles);
                close();
            }

            onClosed: {
                destroy();
            }

            ColumnLayout {
                spacing: Kirigami.Units.gridUnit

                Kirigami.Icon {
                    source: "data-warning-symbolic"

                    Layout.alignment: Qt.AlignCenter
                    Layout.preferredWidth: Kirigami.Units.iconSizes.large
                    Layout.preferredHeight: Kirigami.Units.iconSizes.large
                }
                Kirigami.Heading {
                    text: dialog.subtitle
                    type: Kirigami.Heading.Type.Primary

                    wrapMode: Text.Wrap
                    verticalAlignment: TextEdit.AlignVCenter
                    horizontalAlignment: TextEdit.AlignHCenter

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    // additional spacing
                    Layout.topMargin: Kirigami.Units.gridUnit
                    Layout.bottomMargin: Kirigami.Units.gridUnit
                }
                QQC2.ButtonGroup {
                    id: deleteFilesButtonGroup
                    // None is checked by default.
                    // User much explicitly choose one or another.
                    // Choosing any would also unlock the next stage.
                    checkedButton: null
                }
                RadioItem {
                    id: keepFilesRadioButton
                    Layout.fillWidth: true
                    icon.name: "document-multiple"
                    text: i18n("Keep files")
                    subtitle: i18n("Only user will be deleted from this system, but their home directory is left intact.")
                    QQC2.ButtonGroup.group: deleteFilesButtonGroup
                }
                RadioItem {
                    id: deleteFilesRadioButton
                    Layout.fillWidth: true
                    icon.name: "edit-delete-shred"
                    text: i18n("Delete files")
                    subtitle: i18n("Both user <i>and</i> their home directory will be deleted from this system.")
                    QQC2.ButtonGroup.group: deleteFilesButtonGroup
                }
                QQC2.Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: Kirigami.Units.gridUnit
                    Layout.minimumWidth: Kirigami.Units.gridUnit * 10
                    Layout.preferredHeight: Math.round(implicitHeight * 1.35)
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.35
                    text: i18n("Next")
                    enabled: dialog.canProceed
                    onClicked: dialog.accept();
                }
            }
        }
    }

    Component {
        id: deleteUserConfirmationDialogComponent
        ExtraDimPromptDialog {
            id: dialog

            required property QtObject user
            required property bool alsoDeleteFiles

            readonly property string userName: user.realName !== "" ? user.realName : user.name

            // If user was deleted, user object might be invalidated by the time this signal is emitted.
            signal userDeleted(bool successfully, string userName)

            parent: usersDetailPage.Kirigami.ColumnView.view
            title: i18n("Delete User?")
            subtitle: {
                const warn = alsoDeleteFiles
                    ? i18n("Data will be unrecoverably lost.")
                    : i18n("Data will be left intact.");
                const ask = i18n("Do you really want to continue?");
                return [warn, ask].join("\n\n");
            }

            closePolicy: QQC2.Popup.CloseOnEscape
            showCloseButton: true
            standardButtons: QQC2.Dialog.Cancel

            onAccepted: {
                const name = userName;

                // TODO
                print("YAHAHA", user.uid, alsoDeleteFiles);
                // synchronous call (for now)
                // const successfully = kcm.mainUi.deleteUser(user.uid, alsoDeleteFiles);
                const successfully = Boolean(Math.round(Math.random()));
                // END TODO

                dialog.userDeleted(successfully, name);
                close();
            }

            onClosed: {
                destroy();
            }

            ColumnLayout {
                spacing: Kirigami.Units.gridUnit

                Kirigami.Icon {
                    source: "data-warning-symbolic"

                    Layout.alignment: Qt.AlignCenter
                    Layout.preferredWidth: Kirigami.Units.iconSizes.large
                    Layout.preferredHeight: Kirigami.Units.iconSizes.large
                }
                Kirigami.Heading {
                    text: "Caution"
                    type: Kirigami.Heading.Type.Primary
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                GridLayout {
                    columns: 2
                    rowSpacing: 0
                    columnSpacing: Kirigami.Units.gridUnit

                    Layout.fillWidth: true
                    Layout.margins: Kirigami.Units.gridUnit

                    Kirigami.Avatar {
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 3
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 3
                        Layout.column: 0
                        Layout.row: 0

                        source: dialog.user.face
                        cache: false
                        name: dialog.user.realName
                    }
                    QQC2.Label {
                        text: dialog.userName

                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                        Layout.column: 0
                        Layout.row: 1
                    }
                    QQC2.Label {
                        text: dialog.subtitle
                        wrapMode: Text.Wrap
                        verticalAlignment: Text.AlignTop
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.column: 1
                        Layout.row: 0
                    }
                }

                QQC2.Button {
                    id: deleteUserButton
                    Layout.alignment: Qt.AlignHCenter
                    Layout.minimumWidth: Kirigami.Units.gridUnit * 10
                    Layout.preferredHeight: Math.round(implicitHeight * 1.35)
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.35
                    icon.name: "delete"
                    text: i18n("Delete this user")
                    enabled: dialog.opened && !timer.running
                    opacity: enabled ? 1 : 0.6
                    onClicked: dialog.accept();

                    Behavior on opacity {
                        NumberAnimation {
                            duration: Kirigami.Units.longDuration
                            easing.type: Easing.InOutQuad
                        }
                    }

                }
            }

            property Timer __timer: Timer {
                id: timer
                interval: Kirigami.Units.humanMoment
                running: dialog.opened
            }
        }
    }

    Component {
        id: deleteUserResultDialogComponent
        Kirigami.PromptDialog {
            id: dialog

            required property bool success
            required property string userName

            parent: usersDetailPage.Kirigami.ColumnView.view
            title: dialog.success ? i18n("Success") : i18n("Error")
            subtitle: dialog.success
                ? i18n("Yay \\o/\nUser %1 deleted", dialog.userName)
                : i18n("%1 is indestructible", dialog.userName)

            closePolicy: QQC2.Popup.CloseOnEscape
            showCloseButton: true
            standardButtons: QQC2.Dialog.Close

            onClosed: {
                destroy();
            }

            ColumnLayout {
                spacing: Kirigami.Units.gridUnit

                Kirigami.Icon {
                    source: dialog.success ? "dialog-positive" : "dialog-error"

                    Layout.alignment: Qt.AlignCenter
                    Layout.topMargin: Kirigami.Units.gridUnit
                    Layout.preferredWidth: Kirigami.Units.iconSizes.large
                    Layout.preferredHeight: Kirigami.Units.iconSizes.large
                }
                Kirigami.Heading {
                    text: dialog.subtitle
                    wrapMode: Text.Wrap
                    verticalAlignment: TextEdit.AlignVCenter
                    horizontalAlignment: TextEdit.AlignHCenter

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: Kirigami.Units.gridUnit
                }
            }
        }
    }
}
