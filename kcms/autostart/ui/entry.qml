/*
    SPDX-FileCopyrightText: 2023 Thenujan Sandramohan <sthenujan2002@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.plasma.kcm.autostart

KCM.SimpleKCM {
    id: entryPage

    property Unit unit

    topPadding: 0
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0

    onBackRequested: kcm.pop()

    header: ColumnLayout {
        width: entryPage.width

        spacing: Kirigami.Units.smallSpacing

        Kirigami.InlineMessage {
            id: errorMessage

            Layout.fillWidth: true
            showCloseButton: true

            property bool journalError: false

            Connections {
                target: entryPage.unit
                function onJournalError(message) {
                    errorMessage.type = Kirigami.MessageType.Error;
                    errorMessage.visible = true;
                    errorMessage.text = message;
                    errorMessage.journalError = true;
                }
            }
        }

        Kirigami.FormLayout {
            QQC2.Label {
                Kirigami.FormData.label: i18nc("@label The name of a Systemd unit for an app or script that will autostart", "Name:")
                text: entryPage.unit.description
            }

            QQC2.Label {
                Kirigami.FormData.label: i18nc("@label The current status (e.g. active or inactive) of a Systemd unit for an app or script that will autostart", "Status:")
                text: entryPage.unit.activeState
            }

            QQC2.Label {
                Kirigami.FormData.label: i18nc("@label A date and time follows this text, making a sentence like 'Last activated on: August 7th 11 PM 2023'", "Last activated on:")
                text: entryPage.unit.timeActivated
            }
        }

        QQC2.Button {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Kirigami.Units.largeSpacing
            icon.name: entryPage.unit.activeState === "active" ? "media-playback-stop-symbolic" : "media-playback-start-symbolic"
            text: entryPage.unit.activeState === "active" ? i18nc("@label Stop the Systemd unit for a running process", "Stop") : i18nc("@label Start the Systemd unit for a currently inactive process", "Start")
            onClicked: {
                if (entryPage.unit.activeState === "active") {
                    entryPage.unit.stop();
                } else {
                    entryPage.unit.start();
                }
            }
        }
    }

    QQC2.Label {
        id: sheetLabel
        text: entryPage.unit.logs
                .replace(/Kirigami.Theme.neutralTextColor/g, Kirigami.Theme.neutralTextColor)
                .replace(/Kirigami.Theme.negativeTextColor/g, Kirigami.Theme.negativeTextColor)
                .replace(/Kirigami.Theme.positiveTextColor/g, Kirigami.Theme.positiveTextColor)
        padding: Kirigami.Units.largeSpacing
        wrapMode: Text.WordWrap
        anchors.fill: parent

        background: Rectangle {
            color: Kirigami.Theme.backgroundColor
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false
        }
    }
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)

        visible: entryPage.unit.logs == ''

        icon.name: "data-error-symbolic"
        text: i18n("Unable to load logs. Try refreshing.")

        helpfulAction: Kirigami.Action {
            icon.name: "view-refresh-symbolic"
            text: i18nc("@action:button Refresh entry logs when it failed to load", "Refresh")
            onTriggered: entryPage.unit.reloadLogs()
        }
          
    }

    Timer {
        interval: Kirigami.Units.humanMoment
        running: true
        repeat: true
        onTriggered: {
            // Reloading logs when journal error occurs will cause repeated errorMessage. So we try to avoid it
            if (!errorMessage.journalError) {
                entryPage.unit.reloadLogs()
            }
        }
    }

}
