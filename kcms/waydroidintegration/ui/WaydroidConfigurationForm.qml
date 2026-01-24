/*
 * SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.formcard 1.0 as FormCard
import org.kde.plasma.components 3.0 as PC3
import org.kde.plasma.private.workspace.waydroidintegrationplugin as AIP

ColumnLayout {
    id: root

    FormCard.FormHeader {
        title: i18n("General information")
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: i18n("IP address")
            description: AIP.WaydroidDBusClient.ipAddress
            trailing: PC3.Button {
                visible: AIP.WaydroidDBusClient.ipAddress !== ""
                text: i18n("Copy")
                icon.name: 'edit-copy-symbolic'
                onClicked: AIP.WaydroidDBusClient.copyToClipboard(AIP.WaydroidDBusClient.ipAddress)
            }
        }

        FormCard.FormTextDelegate {
            text: i18n("Waydroid status")
            description: i18n("Running")

            trailing: PC3.Button {
                text: i18n("Stop session")
                onClicked: AIP.WaydroidDBusClient.stopSession()
            }
        }

        FormCard.FormButtonDelegate {
            visible: AIP.WaydroidDBusClient.systemType === AIP.WaydroidDBusClient.Gapps
            text: i18n("Certify my device for Google Play Protect")
            onClicked: kcm.push("WaydroidGooglePlayProtectConfigurationPage.qml")
        }

        FormCard.FormButtonDelegate {
            text: i18n("Installed applications")
            onClicked: kcm.push("WaydroidApplicationsPage.qml")
        }

        FormCard.FormButtonDelegate {
            text: i18n("Reset Waydroid")
            onClicked: confirmDialog.open()
        }

        Kirigami.PromptDialog {
            id: confirmDialog
            title: i18nc("@title:window", "Confirm Waydroid Reset")
            subtitle: i18n("Are you sure you want to reset Waydroid? This is a destructive action, and will wipe all user data.")
            standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

            onAccepted: AIP.WaydroidDBusClient.resetWaydroid()
        }

    }

    // Some information such as IP address can take time to be set by Waydroid
    Timer {
        id: autoRefreshSessionTimer
        interval: 2000
        repeat: true
        running: root.visible
        onTriggered: AIP.WaydroidDBusClient.refreshSessionInfo()
    }

    FormCard.FormHeader {
        title: i18n("Waydroid properties")
    }

    FormCard.FormCard {
        id: infoMessage
        visible: false

        Kirigami.Theme.inherit: false
        Kirigami.Theme.backgroundColor: root.Kirigami.Theme.neutralBackgroundColor

        FormCard.FormTextDelegate {
            text: i18n("May require restarting the Waydroid session to apply")
            textItem.wrapMode: Text.WordWrap
            icon.name: "dialog-warning"
        }
    }

    Connections {
        target: AIP.WaydroidDBusClient

        function onSessionStatusChanged() {
            infoMessage.visible = false
        }
    }

    FormCard.FormCard {
        FormCard.FormSwitchDelegate {
            id: multiWindows
            text: i18n("Windowed apps")
            description: i18n("Run Android apps in dedicated windows")
            checked: AIP.WaydroidDBusClient.multiWindows
            onToggled: {
                AIP.WaydroidDBusClient.multiWindows = checked
                infoMessage.visible = true
            }
        }

        FormCard.FormDelegateSeparator { above: multiWindows; below: suspend }

        FormCard.FormSwitchDelegate {
            id: suspend
            text: i18n("Suspend")
            description: i18n("Pause Waydroid after the display timeout if no Android apps are running")
            checked: AIP.WaydroidDBusClient.suspend
            onToggled: {
                AIP.WaydroidDBusClient.suspend = checked
                infoMessage.visible = true
            }
        }

        FormCard.FormDelegateSeparator { above: suspend; below: uevent }

        FormCard.FormSwitchDelegate {
            id: uevent
            text: i18n("Allow removable device access")
            description: i18n("Let Waydroid directly use removable devices")
            checked: AIP.WaydroidDBusClient.uevent
            onToggled: {
                AIP.WaydroidDBusClient.uevent = checked
                infoMessage.visible = true
            }
        }
    }
}
