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

// TODO turn into MouseArea or MEL for "default action"
// or should that be done where the Popup/ListView is?
ColumnLayout {
    id: notificationItem

    property bool hovered: false
    property int maximumLineCount: 0
    property alias bodyCursorShape: bodyLabel.cursorShape

    property int notificationType

    property alias applicationIconSource: notificationHeading.applicationIconSource
    property alias applicationName: notificationHeading.applicationName

    property string summary
    property var time

    property alias configurable: notificationHeading.configurable
    property alias dismissable: notificationHeading.dismissable
    property alias closable: notificationHeading.closable

    // This isn't an alias because TextEdit RichText adds some HTML tags to it
    property string body
    property alias icon: iconItem.source
    property var urls: []
    property string deviceName

    property int jobState
    property int percentage
    property int error: 0
    property string errorText
    property bool suspendable
    property bool killable

    property QtObject jobDetails
    property bool showDetails

    property alias configureActionLabel: notificationHeading.configureActionLabel
    property var actionNames: []
    property var actionLabels: []

    property int thumbnailLeftPadding: 0
    property int thumbnailRightPadding: 0
    property int thumbnailTopPadding: 0
    property int thumbnailBottomPadding: 0

    signal bodyClicked(var mouse) // TODO bodyClicked?
    signal closeClicked
    signal configureClicked
    signal dismissClicked
    signal actionInvoked(string actionName)
    signal openUrl(string url)

    signal suspendJobClicked
    signal resumeJobClicked
    signal killJobClicked

    onTimeChanged: notificationHeading.updateAgoText()

    spacing: units.smallSpacing

    NotificationHeader {
        id: notificationHeading
        Layout.fillWidth: true

        notificationType: notificationItem.notificationType
        jobState: notificationItem.jobState
        jobDetails: notificationItem.jobDetails

        onConfigureClicked: notificationItem.configureClicked()
        onDismissClicked: notificationItem.dismissClicked()
        onCloseClicked: notificationItem.closeClicked()
    }

    // Notification body
    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            PlasmaExtras.Heading {
                id: summaryLabel
                Layout.fillWidth: true
                Layout.preferredHeight: implicitHeight
                textFormat: Text.PlainText
                maximumLineCount: 3
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                level: 4
                text: {
                    if (notificationItem.notificationType === NotificationManager.Notifications.JobType) {
                        if (notificationItem.jobState === NotificationManager.Notifications.JobStateSuspended) {
                            return i18nc("Job name, e.g. Copying is paused", "%1 (Paused)", notificationItem.summary);
                        } else if (notificationItem.jobState === NotificationManager.Notifications.JobStateStopped) {
                            if (notificationItem.error) {
                                return i18nc("Job name, e.g. Copying has failed", "%1 (Failed)", notificationItem.summary);
                            } else {
                                return i18nc("Job name, e.g. Copying has finished", "%1 (Finished)", notificationItem.summary);
                            }
                        }
                    }
                    return notificationItem.summary;
                }

                // some apps use their app name as summary, avoid showing the same text twice
                // try very hard to match the two
                visible: text !== "" && text.toLocaleLowerCase().trim() !== notificationItem.applicationName.toLocaleLowerCase().trim()

                PlasmaCore.ToolTipArea {
                    anchors.fill: parent
                    active: summaryLabel.truncated
                    textFormat: Text.PlainText
                    subText: summaryLabel.text
                }
            }

            SelectableLabel {
                id: bodyLabel
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true

                Layout.maximumHeight: notificationItem.maximumLineCount > 0
                                      ? (theme.mSize(font).height * notificationItem.maximumLineCount) : -1
                text: notificationItem.body
                // Cannot do text !== "" because RichText adds some HTML tags even when empty
                visible: notificationItem.body !== ""
                onClicked: notificationItem.bodyClicked(mouse)
                onLinkActivated: Qt.openUrlExternally(link)
            }
        }

        PlasmaCore.IconItem {
            id: iconItem
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: units.iconSizes.large
            Layout.preferredHeight: units.iconSizes.large
            usesPlasmaTheme: false
            smooth: true
            // don't show two identical icons
            visible: valid && source != notificationItem.applicationIconSource
        }
    }

    // Job progress reporting
    Loader {
        Layout.fillWidth: true
        active: notificationItem.notificationType === NotificationManager.Notifications.JobType
        sourceComponent: JobItem {
            jobState: notificationItem.jobState
            error: notificationItem.error
            errorText: notificationItem.errorText
            percentage: notificationItem.percentage
            suspendable: notificationItem.suspendable
            killable: notificationItem.killable

            jobDetails: notificationItem.jobDetails
            showDetails: notificationItem.showDetails

            onSuspendJobClicked: notificationItem.suspendJobClicked()
            onResumeJobClicked: notificationItem.resumeJobClicked()
            onKillJobClicked: notificationItem.killJobClicked()

            onOpenUrl: notificationItem.openUrl(url)

            hovered: notificationItem.hovered
        }
    }

    RowLayout {
        Layout.fillWidth: true

        // Notification actions
        Flow { // it's a Flow so it can wrap if too long
            Layout.fillWidth: true
            visible: actionRepeater.count > 0
            spacing: units.smallSpacing
            layoutDirection: Qt.RightToLeft

            Repeater {
                id: actionRepeater
                // HACK We want the actions to be right-aligned but Flow also reverses
                // the order of items, so we manually reverse it here
                model: (notificationItem.actionNames || []).reverse()

                PlasmaComponents.ToolButton {
                    flat: false
                    text: notificationItem.actionLabels[actionRepeater.count - index - 1]
                    Layout.preferredWidth: minimumWidth
                    onClicked: notificationItem.actionInvoked(modelData)
                }
            }
        }
    }

    // thumbnails
    Loader {
        id: thumbnailStripLoader
        Layout.leftMargin: notificationItem.thumbnailLeftPadding
        Layout.rightMargin: notificationItem.thumbnailRightPadding
        Layout.topMargin: notificationItem.thumbnailTopPadding
        Layout.bottomMargin: notificationItem.thumbnailBottomPadding
        Layout.fillWidth: true
        active: notificationItem.urls.length > 0
        visible: active
        sourceComponent: ThumbnailStrip {
            leftPadding: -thumbnailStripLoader.Layout.leftMargin
            rightPadding: -thumbnailStripLoader.Layout.rightMargin
            topPadding: -thumbnailStripLoader.Layout.topMargin
            bottomPadding: -thumbnailStripLoader.Layout.bottomMargin
            urls: notificationItem.urls
            onOpenUrl: notificationItem.openUrl(url)
        }
    }
}
