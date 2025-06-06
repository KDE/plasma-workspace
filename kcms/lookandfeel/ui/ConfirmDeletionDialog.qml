/*
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.PromptDialog {
    id: dialog
    required property string pluginId

    parent: root
    title: i18nc("@title:window", "Delete Permanently")
    subtitle: i18nc("@label", "Do you really want to permanently delete this theme?")
    standardButtons: Kirigami.Dialog.NoButton
    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Delete Permanently")
            icon.name: "delete"
            onTriggered: dialog.accept();
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Cancel")
            icon.name: "dialog-cancel"
            onTriggered: dialog.reject();
        }
    ]

    onAccepted: kcm.removeRow(pluginId, true);
    onClosed: destroy();

    Component.onCompleted: open();
}
