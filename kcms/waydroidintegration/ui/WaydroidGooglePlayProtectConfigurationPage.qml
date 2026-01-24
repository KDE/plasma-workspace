/*
 * SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kcmutils as KCM
import org.kde.kirigamiaddons.formcard 1.0 as FormCard
import org.kde.plasma.private.workspace.waydroidintegrationplugin as AIP

KCM.SimpleKCM {
    id: root

    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    title: i18n("Google Play Protect configuration")

    Component.onCompleted: {
        if (AIP.WaydroidDBusClient.androidId === "") {
            AIP.WaydroidDBusClient.refreshAndroidId()
        }
    }

    WaydroidLoader {
        visible: AIP.WaydroidDBusClient.androidId === ""
        text: i18n("Fetching your Android ID.\nIt can take a few seconds.")
    }

    Connections {
        target: AIP.WaydroidDBusClient

        function onActionFailed(error: string): void {
            inlineMessage.text = error
            inlineMessage.visible = true
            inlineMessage.type = Kirigami.MessageType.Error
        }
    }

    Kirigami.InlineMessage {
        id: inlineMessage

        Layout.fillWidth: true

        visible: false
        showCloseButton: true
    }

    ColumnLayout {
        visible: AIP.WaydroidDBusClient.androidId !== ""
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent
        anchors.leftMargin: Kirigami.Units.largeSpacing
        anchors.right: parent
        anchors.rightMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        Kirigami.PlaceholderMessage {
            explanation: i18n("When launching Waydroid with GAPPS for the first time, you will be notified that the device is not certified for Google Play Protect. To self certify your device, paste the Android ID in the field on the website. Then, give the Google services some minutes to reflect the change and restart Waydroid.")
            Layout.fillWidth: true
        }

        QQC2.Button {
            text: i18nc("@action:button", "Copy Android ID and open the website")
            icon.name: 'edit-copy-symbolic'
            Layout.alignment: Qt.AlignHCenter
            onClicked: {
                AIP.WaydroidDBusClient.copyToClipboard(AIP.WaydroidDBusClient.androidId)
                Qt.openUrlExternally("https://www.google.com/android/uncertified")
            }
        }
    }
}
