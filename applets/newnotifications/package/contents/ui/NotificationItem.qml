/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
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

    property alias applicationIconSource: applicationIconItem.source
    property alias applicationName: applicationNameLabel.text

    property string summary
    property var time

    property alias configurable: configureButton.visible
    property alias dismissable: dismissButton.visible
    property alias closable: closeButton.visible

    // This isn't an alias because TextEdit RichText adds some HTML tags to it
    property string body
    property alias icon: iconItem.source

    property int jobState
    property int percentage
    property int error: 0
    property string errorText
    property bool suspendable
    property bool killable

    property QtObject jobDetails
    property bool showDetails

    property string configureActionLabel
    property var actionNames: []
    property var actionLabels: []

    signal bodyClicked(var mouse) // TODO bodyClicked?
    signal closeClicked
    signal configureClicked
    signal dismissClicked
    signal actionInvoked(string actionName)

    signal suspendJobClicked
    signal resumeJobClicked
    signal killJobClicked

    onTimeChanged: ageLabel.updateText()

    spacing: units.smallSpacing

    // TODO this timer should probably be at a central location
    // so every notification updates simultaneously
    Timer {
        id: updateTimestmapTimer
        interval: 60000
        repeat: true
        running: notificationItem.visible
                 && notificationItem.Window.window
                 && notificationItem.Window.window.visible
        triggeredOnStart: true
        onTriggered: ageLabel.updateText()
    }

    // Notification heading
    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing
        Layout.preferredHeight: Math.max(applicationNameLabel.implicitHeight, units.iconSizes.small)

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
        }

        PlasmaExtras.DescriptiveLabel {
            id: ageLabel
            // the "n minutes ago" text, for jobs we show remaining time instead
            property string agoText: ""
            visible: text !== ""
            text: {
                if (notificationItem.notificationType === NotificationManager.Notifications.JobType
                    && notificationItem.jobState !== NotificationManager.Notifications.JobStateStopped) {
                    var details = notificationItem.jobDetails;
                    if (details && details.speed > 0) {
                        var remaining = details.totalBytes - details.processedBytes;
                        if (remaining > 0) {
                            var eta = remaining / details.speed;
                            // TODO hours?
                            if (eta > 0 && eta < 60 * 90 /*1:30h*/) {
                                if (eta >= 60) {
                                    return i18ncp("minutes remaining, keep short",
                                                  "%1min remaining", "%1min remaining",
                                                  Math.round(eta / 60));
                                } else {
                                    return i18ncp("seconds remaining, keep short",
                                                  "%1s remaining", "%1s remaining", Math.round(eta));
                                }
                            }
                        }
                    }
                    return "";
                }

                return agoText;
            }

            function updateText() {
                var time = notificationItem.time;
                if (time && !isNaN(time.getTime())) {
                    var now = new Date();
                    var deltaMinutes = Math.floor((now.getTime() - time.getTime()) / 1000 / 60);
                    if (deltaMinutes > 0) {
                        agoText = i18ncp("Received minutes ago, keep short", "%1 min ago", "%1 min ago", deltaMinutes);
                        return;
                    }
                }
                agoText = "";
            }
        }

        Item {
            width: headerButtonsRow.width

            RowLayout {
                id: headerButtonsRow
                spacing: units.smallSpacing * 2
                anchors.verticalCenter: parent.verticalCenter

                // These aren't ToolButtons so they can be perfectly aligned
                // FIXME fix layout overlap
                HeaderButton {
                    id: configureButton
                    tooltip: notificationItem.configureActionLabel || i18n("Configure")
                    iconSource: "configure"
                    visible: false
                    onClicked: notificationItem.configureClicked()
                }

                HeaderButton {
                    id: dismissButton
                    tooltip: i18n("Hide")
                    // FIXME proper icon, perhaps from widgets/configuration-icon
                    iconSource: "file-zoom-out"
                    visible: false
                    onClicked: notificationItem.dismissClicked()
                }

                HeaderButton {
                    id: closeButton
                    tooltip: i18n("Close")
                    iconSource: "window-close"
                    visible: false
                    onClicked: notificationItem.closeClicked()
                }
            }
        }
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
                wrapMode: Text.NoWrap
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
                visible: text !== "" && text.toLocaleLowerCase().trim() !== applicationNameLabel.text.toLocaleLowerCase().trim()

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
            visible: valid && source != applicationIconItem.source
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

            hovered: notificationItem.hovered
        }
    }

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
