// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Arjen Hiemstra <ahiemstra@quantumproductions.info>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.ki18n as KI18n

import org.kde.private.kcms.union as Private

KCM.GridViewKCM {
    id: root

    view.model: Private.StylesModel { }
    view.implicitCellWidth: Kirigami.Units.gridUnit * 21
    view.implicitCellHeight: Kirigami.Units.gridUnit * 15
    view.currentIndex: view.model.indexOf(kcm.currentStyleId)

    KCM.SettingStateBinding {
        configObject: kcm.settings
        settingName: "style"
    }

    actions: [
        Kirigami.Action {
            icon.name: "document-import-symbolic"
            text: i18n.i18nc("@action:button", "Install From File…")
            onTriggered: installDialog.open()
        },
        Kirigami.Action {
            icon.name: "get-hot-new-stuff-symbolic"
            text: i18n.i18nc("@action:button", "Get New…")
            enabled: false;
        }
    ]

    headerPaddingEnabled: false // Let the InlineMessage touch the edges
    header: Kirigami.InlineMessage {
        id: message

        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Error
        showCloseButton: true
        visible: false

        Connections {
            target: kcm

            function onShowError(error) {
                message.text = error
                message.type = Kirigami.MessageType.Error
                message.visible = true
            }

            function onShowInfo(info) {
                message.text = info
                message.type = Kirigami.MessageType.Information
                message.visible = true
            }
        }
    }

    view.delegate: KCM.GridDelegate {
        id: delegate

        required property string styleId
        required property string name
        required property string description
        required property bool canUninstall

        text: name
        toolTip: description

        thumbnailAvailable: true
        thumbnail: ThumbnailItem {
            anchors.fill: parent
            styleId: delegate.styleId

            MouseArea {
                anchors.fill: parent

                onClicked: kcm.currentStyleId = delegate.styleId
                onDoubleClicked: kcm.save()
            }
        }

        actions: [
            Kirigami.Action {
                icon.name: "delete-symbolic"
                tooltip: i18n.i18nc("@action", "Uninstall…")
                enabled: delegate.canUninstall
                onTriggered: {
                    uninstallDialog.styleId = delegate.styleId
                    uninstallDialog.styleName = delegate.name
                    uninstallDialog.open()
                }
            }
        ]

        onClicked: {
            kcm.currentStyleId = styleId
        }

        onDoubleClicked: {
            kcm.save()
        }
    }

    FolderDialog {
        id: installDialog

        title: i18n.i18nc("@title:dialog", "Choose Style to Install")
        acceptLabel: i18n.i18nc("@action:button", "Install")
        options: FileDialog.ReadOnly

        onAccepted: {
            if (kcm.installStyle(selectedFolder)) {
                root.view.model.refresh()
            }
        }
    }

    MessageDialog {
        id: uninstallDialog

        property string styleId
        property string styleName

        title: i18n.i18nc("@title:dialog", "Uninstall")
        text: i18n.i18nc("@message:dialog", `Are you sure you want to uninstall style ${styleName}? This cannot be undone.`)
        buttons: MessageDialog.Yes | MessageDialog.No

        onAccepted: {
            if (kcm.uninstallStyle(uninstallDialog.styleId)) {
                root.view.model.refresh()
            }
        }
    }

    KI18n.KI18nContext {
        id: i18n
        translationDomain: "kcm_union_style"
    }
}
