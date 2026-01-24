/*
 * SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kcmutils as KCM
import org.kde.kirigamiaddons.formcard 1.0 as FormCard
import org.kde.plasma.components 3.0 as PC3
import org.kde.plasma.private.workspace.waydroidintegrationplugin as AIP

KCM.SimpleKCM {
    id: root

    title: i18n("Waydroid Integration")

    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    ColumnLayout {
        visible: AIP.WaydroidDBusClient.status === AIP.WaydroidDBusClient.NotSupported
        anchors.centerIn: parent
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            text: i18n("Waydroid is not installed")
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter

        }

        PC3.Button {
            text: i18n("Check installation")
            Layout.alignment: Qt.AlignHCenter
            onClicked: AIP.WaydroidDBusClient.refreshSupportsInfo()
        }
    }

    WaydroidInitialConfigurationForm {
        visible: AIP.WaydroidDBusClient.status === AIP.WaydroidDBusClient.NotInitialized
    }

    WaydroidDownloadStatus {
        id: downloadStatus
        visible: AIP.WaydroidDBusClient.status === AIP.WaydroidDBusClient.Initializing
        text: i18n("Downloading Android and vendor images.\nIt can take a few minutes.")

        Connections {
            target: AIP.WaydroidDBusClient

            function onDownloadStatusChanged(downloaded, total, speed) {
                downloadStatus.downloaded = downloaded
                downloadStatus.total = total
                downloadStatus.speed = speed
            }
        }
    }

    WaydroidLoader {
        visible: AIP.WaydroidDBusClient.status === AIP.WaydroidDBusClient.Resetting
        text: i18n("Waydroid is resetting.\nIt can take a few seconds.")
    }

    ColumnLayout {
        visible: AIP.WaydroidDBusClient.status === AIP.WaydroidDBusClient.Initialized && AIP.WaydroidDBusClient.sessionStatus === AIP.WaydroidDBusClient.SessionStopped
        anchors.centerIn: parent
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            text: i18n("The Waydroid session is not running.")
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
        }

        PC3.Button {
            text: i18n("Start the session")
            Layout.alignment: Qt.AlignHCenter
            onClicked: AIP.WaydroidDBusClient.startSession()
        }
    }

    WaydroidLoader {
        visible: AIP.WaydroidDBusClient.status === AIP.WaydroidDBusClient.Initialized && AIP.WaydroidDBusClient.sessionStatus === AIP.WaydroidDBusClient.SessionStarting
        text: i18n("Waydroid session is starting.\nIt can take a few seconds.")
    }

    WaydroidConfigurationForm {
        visible: AIP.WaydroidDBusClient.status === AIP.WaydroidDBusClient.Initialized && AIP.WaydroidDBusClient.sessionStatus === AIP.WaydroidDBusClient.SessionRunning
    }

    Connections {
        target: AIP.WaydroidDBusClient

        function onErrorOccurred(title, message) {
            kcm.push("WaydroidErrorPage.qml", { title, message })
        }
    }
}
