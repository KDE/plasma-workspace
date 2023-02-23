/*
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami

Kirigami.PromptDialog {
    id: root

    required property QtObject user

    title: i18n("Change Wallet Password?")

    closePolicy: QQC2.Popup.CloseOnEscape
    showCloseButton: false
    standardButtons: QQC2.Dialog.NoButton

    customFooterActions: [
        Kirigami.Action {
            text: i18n("Change Wallet Password")
            icon.name: "lock"
            onTriggered: {
                root.user.changeWalletPassword()
                root.close()
            }
        },
        Kirigami.Action {
            text: i18n("Leave Unchanged")
            icon.name: "dialog-cancel"
            onTriggered: {
                root.close()
            }
        }
    ]

    ColumnLayout {
        id: baseLayout
        Layout.preferredWidth: Kirigami.Units.gridUnit * 27
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            text: xi18nc("@info", "Now that you have changed your login password, you may also want to change the password on your default KWallet to match it.")
            wrapMode: Text.Wrap
        }

        Kirigami.LinkButton {
            text: i18n("What is KWallet?")
            onClicked: {
                whatIsKWalletExplanation.visible = !whatIsKWalletExplanation.visible
            }
        }

        QQC2.Label {
            id: whatIsKWalletExplanation
            Layout.fillWidth: true
            visible: false
            text: i18n("KWallet is a password manager that stores your passwords for wireless networks and other encrypted resources. It is locked with its own password which differs from your login password. If the two passwords match, it can be unlocked at login automatically so you don't have to enter the KWallet password yourself.")
            wrapMode: Text.Wrap
        }
    }
}
