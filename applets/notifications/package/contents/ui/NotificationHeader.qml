/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.8
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.notificationmanager 1.0 as NotificationManager

import org.kde.kcoreaddons 1.0 as KCoreAddons

import "global"

RowLayout {
    id: notificationHeading

    property int notificationType

    property alias applicationIconSource: applicationIconItem.source
    property string applicationName
    property string deviceName

    property string configureActionLabel

    property alias configurable: configureButton.visible
    property alias dismissable: dismissButton.visible
    property alias closable: closeButton.visible

    property var time

    property int jobState
    property QtObject jobDetails

    signal configureClicked
    signal dismissClicked
    signal closeClicked

    // notification created/updated time changed
    onTimeChanged: updateAgoText()

    function updateAgoText() {
        ageLabel.agoText = ageLabel.generateAgoText();
    }

    spacing: units.smallSpacing
    Layout.preferredHeight: Math.max(applicationNameLabel.implicitHeight, units.iconSizes.small)

    Connections {
        target: Globals
        // clock time changed
        // TODO should we do this only when actually visible/expanded?
        onTimeChanged: notificationHeading.updateAgoText()
    }

    PlasmaCore.IconItem {
        id: applicationIconItem
        Layout.preferredWidth: units.iconSizes.small
        Layout.preferredHeight: units.iconSizes.small
        usesPlasmaTheme: false
        visible: valid
    }

    PlasmaExtras.DescriptiveLabel {
        id: applicationNameLabel
        Layout.fillWidth: true
        textFormat: Text.PlainText
        elide: Text.ElideRight
        text: notificationHeading.applicationName + (notificationHeading.deviceName ? " · " + notificationHeading.deviceName : "")
    }

    PlasmaExtras.DescriptiveLabel {
        id: ageLabel

        // the "n minutes ago" text, for jobs we show remaining time instead
        // updated periodically by a Timer hence this property with generate() function
        property string agoText: ""
        visible: text !== ""
        text: generateRemainingText() || agoText

        function generateAgoText() {
            if (!time || isNaN(time.getTime()) || notificationHeading.jobState === NotificationManager.Notifications.JobStateRunning) {
                return "";
            }

            var now = new Date();
            var deltaMinutes = Math.floor((now.getTime() - time.getTime()) / 1000 / 60);
            if (deltaMinutes < 1) {
                return "";
            }

            // Received less than an hour ago, show relative minutes
            if (deltaMinutes < 60) {
                return i18ncp("Notification was added minutes ago, keep short", "%1 min ago", "%1 min ago", deltaMinutes);
            }
            // Received less than a day ago, show time, 23 hours so the time isn't ambiguous between today and yesterday
            if (deltaMinutes < 60 * 23) {
                return Qt.formatTime(time, Qt.locale().timeFormat(Locale.ShortFormat).replace(/.ss?/i, ""));
            }

            // Otherwise show relative date (Yesterday, "Last Sunday", or just date if too far in the past)
            return KCoreAddons.Format.formatRelativeDate(time, Locale.ShortFormat);
        }

        function generateRemainingText() {
            if (notificationHeading.notificationType !== NotificationManager.Notifications.JobType
                || notificationHeading.jobState === NotificationManager.Notifications.JobStateStopped) {
                return "";
            }

            var details = notificationHeading.jobDetails;
            if (!details || !details.speed) {
                return "";
            }

            var remaining = details.totalBytes - details.processedBytes;
            if (remaining <= 0) {
                return "";
            }

            var eta = remaining / details.speed;
            if (!eta) {
                return "";
            }

            if (eta < 60) { // 1 minute
                return i18ncp("seconds remaining, keep short",
                              "%1s remaining", "%1s remaining", Math.round(eta));
            }
            if (eta < 60 * 60) {// 1 hour
                return i18ncp("minutes remaining, keep short",
                              "%1min remaining", "%1min remaining",
                              Math.round(eta / 60));
            }
            if (eta < 60 * 60 * 5) { // 5 hours max, if it takes even longer there's no real point in shoing that
                return i18ncp("hours remaining, keep short",
                              "%1h remaining", "%1h remaining",
                              Math.round(eta / 60 / 60));
            }

            return "";
        }

        PlasmaCore.ToolTipArea {
            anchors.fill: parent
            active: ageLabel.agoText !== ""
            subText: notificationHeading.time ? notificationHeading.time.toLocaleString(Qt.locale(), Locale.LongFormat) : ""
        }
    }

    RowLayout {
        id: headerButtonsRow
        spacing: units.smallSpacing * 2
        Layout.leftMargin: units.smallSpacing

        // These aren't ToolButtons so they can be perfectly aligned
        // FIXME fix layout overlap
        HeaderButton {
            id: configureButton
            tooltip: notificationHeading.configureActionLabel || i18n("Configure")
            iconSource: "configure"
            visible: false
            onClicked: notificationHeading.configureClicked()
        }

        HeaderButton {
            id: dismissButton
            tooltip: i18n("Hide")
            // FIXME proper icon, perhaps from widgets/configuration-icon
            iconSource: "file-zoom-out"
            visible: false
            onClicked: notificationHeading.dismissClicked()
        }

        HeaderButton {
            id: closeButton
            tooltip: i18n("Close")
            iconSource: "window-close"
            visible: false
            onClicked: notificationHeading.closeClicked()
        }
    }
}
