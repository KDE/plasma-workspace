/*
    SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.10
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.11
import org.kde.kirigami 2.13 as Kirigami
import QtQuick.Dialogs 1.3
import org.kde.kcm 1.2 as KCM
import org.kde.plasma.kcm.autostart 1.0

KCM.ScrollViewKCM {

    id: root

    implicitHeight: Kirigami.Units.gridUnit * 28
    implicitWidth: Kirigami.Units.gridUnit * 28

    header: Kirigami.InlineMessage {
        id: errorMessage
        type: Kirigami.MessageType.Error
        showCloseButton: true

        Connections {
            target: kcm.model
            function onError(message) {
                errorMessage.visible = true
                errorMessage.text = message
            }
        }
    }

    view: ListView {
        clip: true
        model: kcm.model

        delegate: Kirigami.SwipeListItem {

            RowLayout {
                CheckBox {
                    id: itemChecked
                    Layout.leftMargin: Kirigami.Units.gridUnit
                    Layout.alignment: Qt.AlignVCenter
                    checked: model.enabled
                    visible: model.source === AutostartModel.XdgAutoStart
                    onClicked: kcm.model.toggleApplication(model.index);
                }
                Kirigami.Icon {
                    id: appIcon
                    source: model.iconName
                    width: Kirigami.Units.iconSizes.medium
                    height: Kirigami.Units.iconSizes.medium
                }

                Label {
                    Layout.fillWidth: true
                    height: appIcon.height
                    text: model.name
                    elide: Text.ElideRight
                }
            }

            actions: [
                Kirigami.Action {
                    text: i18n("Properties")
                    icon.name: "document-properties"
                    onTriggered: kcm.model.editApplication(model.index, root)
                    visible: model.source === AutostartModel.XdgAutoStart || model.source === AutostartModel.XdgScripts
                },
                Kirigami.Action {
                    text: i18n("Remove")
                    icon.name: "list-remove"
                    onTriggered: kcm.model.removeEntry(model.index)
                    visible: model.isUser
                }
            ]
        }

        section.property: "source"
        section.delegate: Kirigami.ListSectionHeader {
            text: {
                if (section == AutostartModel.XdgAutoStart) {
                    return i18n("Applications")
                }
                if (section == AutostartModel.XdgScripts) {
                    return i18n("Login Scripts")
                }
                if (section == AutostartModel.PlasmaEnvScripts) {
                    return i18n("Pre-startup Scripts")
                }
                if (section == AutostartModel.PlasmaShutdown) {
                    return i18n("Logout Scripts")
                }
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: parent.count === 0
            text: i18n("No user-specified autostart items")
            explanation: xi18nc("@info", "Click the <interface>Add…</interface> button below to add some")
        }
    }

    footer: Row {
        spacing: Kirigami.Units.largeSpacing

        Loader {
            id: loginFileDialogLoader

            active: false

            sourceComponent: FileDialog {
                id: loginFileDialog
                title: i18n("Choose Login Script")
                folder: shortcuts.home
                selectMultiple: false
                onAccepted: {
                    kcm.model.addScript(loginFileDialog.fileUrl, AutostartModel.XdgScripts)
                    loginFileDialogLoader.active = false
                }

                onRejected: loginFileDialogLoader.active = false

                Component.onCompleted: open()
            }
        }

        Loader {
            id: logoutFileDialogLoader

            active: false

            sourceComponent: FileDialog {
                id: logoutFileDialog
                title: i18n("Choose Logout Script")
                folder: shortcuts.home
                selectMultiple: false
                onAccepted: {
                    kcm.model.addScript(logoutFileDialog.fileUrl, AutostartModel.PlasmaShutdown)
                    logoutFileDialogLoader.active = false
                }

                onRejected: logoutFileDialogLoader.active = false

                Component.onCompleted: open()
            }
        }

        Button {
            id: menuButton

            icon.name: "list-add"
            text: i18n("Add…")

            checkable: true
            checked: menu.opened

            onPressed: {
                // Appear above the button, not below it, since the button is at
                // the bottom of the window and QQC2 items can't leave the window

                // HACK: since we want to position the menu above the button,
                // we need to know the menu's height, but it only has a height
                // after the first time it's been shown, so until then, we need
                // to provide an artificially-synthesized-and-hopefully-good-enough
                // height value
                var menuHeight = menu.height && menu.height > 0 ? menu.height : Kirigami.Units.gridUnit * 3
                menu.popup(menuButton, 0, -menuHeight)
            }
        }

        Menu {
            id: menu

            modal: true
            dim: false

            MenuItem {
                text: i18n("Add Application…")
                icon.name: "list-add"

                onClicked: kcm.model.showApplicationDialog(root)
            }
            MenuItem {
                text: i18n("Add Login Script…")
                icon.name: "list-add"

                onClicked: loginFileDialogLoader.active = true
            }
            MenuItem {
                text: i18n("Add Logout Script…")
                icon.name: "list-add"

                onClicked: logoutFileDialogLoader.active = true
            }
        }
    }
}
