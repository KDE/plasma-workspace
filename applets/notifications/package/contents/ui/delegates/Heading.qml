/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Window

import org.kde.kirigami as Kirigami
import org.kde.notificationmanager as NotificationManager


Kirigami.Heading {
    id: summaryLabel

    property string summary
    property string applicationName
    property int notificationType
    property int jobState

    Layout.fillWidth: true
    Layout.preferredHeight: implicitHeight
    textFormat: Text.PlainText
    maximumLineCount: 3
    wrapMode: Text.WordWrap
    elide: Text.ElideRight
    level: 4
    // Give it a bit more visual prominence than the app name in the header
    type: Kirigami.Heading.Type.Primary
    text: {
        if (notificationType === NotificationManager.Notifications.JobType) {
            if (jobState === NotificationManager.Notifications.JobStateSuspended) {
                if (summary) {
                    return i18ndc("plasma_applet_org.kde.plasma.notifications", "Job name, e.g. Copying is paused", "%1 (Paused)", summary);
                }
            } else if (jobState === NotificationManager.Notifications.JobStateStopped) {
                if (jobError) {
                    if (summary) {
                        return i18ndc("plasma_applet_org.kde.plasma.notifications", "Job name, e.g. Copying has failed", "%1 (Failed)", summary);
                    } else {
                        return i18nd("plasma_applet_org.kde.plasma.notifications", "Job Failed");
                    }
                } else {
                    if (summary) {
                        return i18ndc("plasma_applet_org.kde.plasma.notifications", "Job name, e.g. Copying has finished", "%1 (Finished)", summary);
                    } else {
                        return i18nd("plasma_applet_org.kde.plasma.notifications", "Job Finished");
                    }
                }
            }
        }
        // some apps use their app name as summary, avoid showing the same text twice
        // try very hard to match the two
        if (summary && summary.toLocaleLowerCase().trim() != applicationName.toLocaleLowerCase().trim()) {
            return summary;
        }
        return "";
    }
    visible: text !== ""

    TextMetrics {
        id: summaryLabelTextMetrics
        font: summaryLabel.font
        text: summaryLabel.text
    }
}
