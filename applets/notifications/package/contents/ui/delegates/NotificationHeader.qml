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

import org.kde.quickcharts as Charts

import "../global"

RowLayout {
    id: notificationHeading

    property ModelInterface modelInterface: ModelInterface {}
    property alias closeButtonTooltip: closeButtonToolTip.text

    Connections {
        target: modelInterface
        function onTimeChanged() {
            notificationHeading.updateAgoText()
        }
    }
    function updateAgoText() {
        ageLabel.agoText = ageLabel.generateAgoText();
    }

    spacing: Kirigami.Units.smallSpacing

    Component.onCompleted: updateAgoText()

    Connections {
        target: Globals
        // clock time changed
        function onTimeChanged() {
            notificationHeading.updateAgoText()
        }
    }

    Kirigami.Icon {
        id: applicationIconItem
        Layout.preferredWidth: Kirigami.Units.iconSizes.small
        Layout.preferredHeight: Kirigami.Units.iconSizes.small
        source: modelInterface.applicationIconSource
        visible: valid
    }

    Kirigami.Heading {
        id: applicationNameLabel
        Layout.fillWidth: true
        level: 5
        opacity: 0.9
        textFormat: Text.PlainText
        elide: Text.ElideMiddle
        maximumLineCount: 2
        text: modelInterface.applicationName + (modelInterface.originName ? " · " + modelInterface.originName : "")
    }

    Item {
        id: spacer
        Layout.fillWidth: true
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

        function generateAgoText() {
            const time = modelInterface.time;
            if (!time || isNaN(time.getTime())
                    || modelInterface.jobState === NotificationManager.Notifications.JobStateRunning
                    || modelInterface.jobState === NotificationManager.Notifications.JobStateSuspended) {
                return "";
            }

            var deltaMinutes = Math.floor((Date.now() - time.getTime()) / 1000 / 60);
            if (deltaMinutes < 1) {
                // "Just now" is implied by
                return modelInterface.inHistory
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

        function generateRemainingText() {
            if (modelInterface.notificationType !== NotificationManager.Notifications.JobType
                || modelInterface.jobState !== NotificationManager.Notifications.JobStateRunning) {
                return "";
            }

            var details = modelInterface.jobDetails;
            if (!details || !details.speed) {
                return "";
            }

            var remaining = details.totalBytes - details.processedBytes;
            if (remaining <= 0) {
                return "";
            }

            var eta = remaining / details.speed;
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
            subText: modelInterface.time ? modelInterface.time.toLocaleString(Qt.locale(), Locale.LongFormat) : ""
        }
    }

    PlasmaComponents3.ToolButton {
        id: configureButton
        icon.name: "configure"
        visible: modelInterface.configurable

        display: PlasmaComponents3.AbstractButton.IconOnly
        text: modelInterface.configureActionLabel || i18nd("plasma_applet_org.kde.plasma.notifications", "Configure")
        Accessible.description: applicationNameLabel.text

        onClicked: modelInterface.configureClicked()

        PlasmaComponents3.ToolTip {
            text: parent.text
        }
    }

    PlasmaComponents3.ToolButton {
        id: dismissButton
        icon.name: modelInterface.dismissed ? "window-restore" : "window-minimize"
        visible: modelInterface.dismissable

        display: PlasmaComponents3.AbstractButton.IconOnly
        text: modelInterface.dismissed
            ? i18ndc("plasma_applet_org.kde.plasma.notifications", "Opposite of minimize", "Restore")
            : i18nd("plasma_applet_org.kde.plasma.notifications", "Minimize")
        Accessible.description: applicationNameLabel.text

        onClicked: modelInterface.dismissClicked()

        PlasmaComponents3.ToolTip {
            text: parent.text
        }
    }

    PlasmaComponents3.ToolButton {
        id: closeButton
        visible: modelInterface.closable
        icon.name: "window-close"

        display: PlasmaComponents3.AbstractButton.IconOnly
        text: closeButtonToolTip.text
        Accessible.description: applicationNameLabel.text

        onClicked: modelInterface.closeClicked()

        PlasmaComponents3.ToolTip {
            id: closeButtonToolTip
            text: modelInterface.closeButtonToolTip || i18nd("plasma_applet_org.kde.plasma.notifications", "Close")
        }

        Charts.PieChart {
            id: chart
            anchors.fill: parent.contentItem
            anchors.margins: 1

            opacity: (modelInterface.remainingTime > 0 && modelInterface.remainingTime < modelInterface.timeout) ? 1 : 0
            Behavior on opacity {
                NumberAnimation { duration: Kirigami.Units.longDuration }
            }

            range { from: 0; to: modelInterface.timeout; automatic: false }

            valueSources: Charts.SingleValueSource { value: modelInterface.remainingTime }
            range { from: 0; to: 2000; automatic: false }

            colorSource: Charts.SingleValueSource { value: Kirigami.Theme.highlightColor }

            thickness: 5

            transform: Scale { origin.x: chart.width / 2; xScale: -1 }
        }
    }

    states: [
        State {
            when: modelInterface.inGroup
            PropertyChanges {
                target: applicationIconItem
                source: ""
            }
            PropertyChanges {
                target: applicationNameLabel
                visible: false
            }
        }

    ]
}
