/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

import org.kde.notificationmanager as NotificationManager

import org.kde.coreaddons as KCoreAddons

import plasma.applet.org.kde.plasma.notifications as Notifications

RowLayout {
    id: headingButtons

    required property ModelInterface modelInterface
    property alias closeButtonTooltip: closeButtonToolTip.text

    readonly property string __applicationName: modelInterface.applicationName + (modelInterface.originName ? " · " + modelInterface.originName : "")

    Connections {
        target: headingButtons.modelInterface

        function onTimeChanged(): void {
            headingButtons.updateAgoText();
        }
    }

    function updateAgoText(): void {
        ageLabel.agoText = ageLabel.generateAgoText();
    }

    spacing: Kirigami.Units.smallSpacing

    Component.onCompleted: updateAgoText()

    Connections {
        target: Notifications.Globals

        // clock time changed
        function onTimeChanged(): void {
            headingButtons.updateAgoText();
        }
    }

    Kirigami.Heading {
        id: ageLabel

        // the "n minutes ago" text, for jobs we show remaining time instead
        // updated periodically by a Timer hence this property with generate() function
        property string agoText: ""
        visible: text !== ""
        level: 5
        opacity: 0.9
        wrapMode: Text.NoWrap
        text: generateRemainingText() || agoText
        textFormat: Text.PlainText

        // Returns a localized string describing how long ago this notification was sent (or updated).
        function generateAgoText(): string {
            const time = headingButtons.modelInterface.time;
            if (!time || isNaN(time.getTime())
                    || headingButtons.modelInterface.jobState === NotificationManager.Notifications.JobStateRunning
                    || headingButtons.modelInterface.jobState === NotificationManager.Notifications.JobStateSuspended) {
                return "";
            }

            let deltaMinutes = Math.floor((Date.now() - time.getTime()) / 1000 / 60);
            if (deltaMinutes < 1) {
                // "Just now" is implied by
                return headingButtons.modelInterface.inHistory
                    ? i18ndc("plasma_applet_org.kde.plasma.notifications", "Notification was added less than a minute ago, keep short", "Just now")
                    : "";
            }

            // Received less than an hour ago, show relative minutes
            if (deltaMinutes < 60) {
                return i18ndcp("plasma_applet_org.kde.plasma.notifications", "Notification was added minutes ago, keep short", "%1 min ago", "%1 min ago", deltaMinutes);
            }
            // Received less than a day ago, show time, 22 hours so the time isn't as ambiguous between today and yesterday
            if (deltaMinutes < 60 * 22) {
                return Qt.formatTime(time, Qt.locale().timeFormat(Locale.ShortFormat).replace(/.ss?/i, ""));
            }

            // Otherwise show relative date (Yesterday, "Last Sunday", or just date if too far in the past)
            return KCoreAddons.Format.formatRelativeDate(time, Locale.ShortFormat);
        }

        // Returns a localized string describing how much time is remaining to complete this notification's job.
        function generateRemainingText(): string {
            if (headingButtons.modelInterface.notificationType !== NotificationManager.Notifications.JobType
                || headingButtons.modelInterface.jobState !== NotificationManager.Notifications.JobStateRunning) {
                return "";
            }

            let details = headingButtons.modelInterface.jobDetails;
            if (!details || !details.speed) {
                return "";
            }

            let remaining = details.totalBytes - details.processedBytes;
            if (remaining <= 0) {
                return "";
            }

            let eta = remaining / details.speed;
            if (eta < 0.5) { // Avoid showing "0 seconds remaining"
                return "";
            }

            if (eta < 60) { // 1 minute
                return i18ndcp("plasma_applet_org.kde.plasma.notifications", "seconds remaining, keep short",
                              "%1 s remaining", "%1 s remaining", Math.round(eta));
            }
            if (eta < 60 * 60) {// 1 hour
                return i18ndcp("plasma_applet_org.kde.plasma.notifications", "minutes remaining, keep short",
                              "%1 min remaining", "%1 min remaining",
                              Math.round(eta / 60));
            }
            if (eta < 60 * 60 * 5) { // 5 hours max, if it takes even longer there's no real point in showing that
                return i18ndcp("plasma_applet_org.kde.plasma.notifications", "hours remaining, keep short",
                              "%1 h remaining", "%1 h remaining",
                              Math.round(eta / 60 / 60));
            }

            return "";
        }

        PlasmaCore.ToolTipArea {
            anchors.fill: parent
            active: ageLabel.agoText !== ""
            subText: headingButtons.modelInterface.time ? headingButtons.modelInterface.time.toLocaleString(Qt.locale(), Locale.LongFormat) : ""
        }
    }

    PlasmaComponents3.ToolButton {
        id: configureButton
        icon.name: "configure"
        visible: headingButtons.modelInterface.configurable

        display: PlasmaComponents3.AbstractButton.IconOnly
        text: headingButtons.modelInterface.configureActionLabel || i18nd("plasma_applet_org.kde.plasma.notifications", "Configure")
        Accessible.description: headingButtons.__applicationName

        onClicked: headingButtons.modelInterface.configureClicked()

        PlasmaComponents3.ToolTip {
            text: configureButton.text
        }
    }

    PlasmaComponents3.ToolButton {
        id: dismissButton
        icon.name: headingButtons.modelInterface.dismissed ? "window-restore" : "window-minimize"
        visible: headingButtons.modelInterface.dismissable

        display: PlasmaComponents3.AbstractButton.IconOnly
        text: headingButtons.modelInterface.dismissed
            ? i18ndc("plasma_applet_org.kde.plasma.notifications", "Opposite of minimize", "Restore")
            : i18nd("plasma_applet_org.kde.plasma.notifications", "Minimize")
        Accessible.description: headingButtons.__applicationName

        onClicked: headingButtons.modelInterface.dismissClicked()

        PlasmaComponents3.ToolTip {
            text: dismissButton.text
        }
    }

    PlasmaComponents3.ToolButton {
        id: closeButton
        visible: headingButtons.modelInterface.closable
        icon.name: "window-close"

        display: PlasmaComponents3.AbstractButton.IconOnly
        text: closeButtonToolTip.text
        Accessible.description: headingButtons.__applicationName

        onClicked: headingButtons.modelInterface.closeClicked()

        PlasmaComponents3.ToolTip {
            id: closeButtonToolTip
            text: headingButtons.modelInterface.closeButtonToolTip || i18nd("plasma_applet_org.kde.plasma.notifications", "Close")
        }
    }
}
