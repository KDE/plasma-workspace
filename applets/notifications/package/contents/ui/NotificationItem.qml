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
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kquickcontrolsaddons 2.0 as KQCAddons

import org.kde.notificationmanager 1.0 as NotificationManager

ColumnLayout {
    id: notificationItem

    property bool hovered: false
    property int maximumLineCount: 0
    property alias bodyCursorShape: bodyLabel.cursorShape

    property int notificationType

    property bool inGroup: false
    property bool inHistory: false

    property alias applicationIconSource: notificationHeading.applicationIconSource
    property alias applicationName: notificationHeading.applicationName
    property alias originName: notificationHeading.originName

    property string summary
    property alias time: notificationHeading.time

    property alias configurable: notificationHeading.configurable
    property alias dismissable: notificationHeading.dismissable
    property alias dismissed: notificationHeading.dismissed
    property alias closable: notificationHeading.closable

    // This isn't an alias because TextEdit RichText adds some HTML tags to it
    property string body
    property var icon
    property var urls: []

    property int jobState
    property int percentage
    property int jobError: 0
    property bool suspendable
    property bool killable

    property QtObject jobDetails
    property bool showDetails

    property alias configureActionLabel: notificationHeading.configureActionLabel
    property var actionNames: []
    property var actionLabels: []

    property bool hasReplyAction
    property string replyActionLabel
    property string replyPlaceholderText
    property string replySubmitButtonText
    property string replySubmitButtonIconName

    property int headingLeftPadding: 0
    property int headingRightPadding: 0

    property int thumbnailLeftPadding: 0
    property int thumbnailRightPadding: 0
    property int thumbnailTopPadding: 0
    property int thumbnailBottomPadding: 0

    property alias timeout: notificationHeading.timeout
    property alias remainingTime: notificationHeading.remainingTime

    readonly property bool menuOpen: bodyLabel.contextMenu !== null
                                     || (thumbnailStripLoader.item && thumbnailStripLoader.item.menuOpen)
                                     || (jobLoader.item && jobLoader.item.menuOpen)

    readonly property bool dragging: (thumbnailStripLoader.item && thumbnailStripLoader.item.dragging)
                                        || (jobLoader.item && jobLoader.item.dragging)
    property bool replying: false

    signal bodyClicked(var mouse)
    signal closeClicked
    signal configureClicked
    signal dismissClicked
    signal actionInvoked(string actionName)
    signal replied(string text)
    signal openUrl(string url)
    signal fileActionInvoked

    signal suspendJobClicked
    signal resumeJobClicked
    signal killJobClicked

    spacing: units.smallSpacing

    Item {
        id: headingElement
        Layout.fillWidth: true
        Layout.preferredHeight: notificationHeading.implicitHeight
        Layout.preferredWidth: notificationHeading.implicitWidth
        Layout.bottomMargin: -parent.spacing

        PlasmaCore.FrameSvgItem {
            imagePath: "widgets/plasmoidheading"
            prefix: "header"
            anchors {
                fill: parent
                leftMargin: -margins.left
                rightMargin: -margins.right
            }
            visible: !notificationItem.inHistory && fromCurrentTheme
        }

        NotificationHeader {
            id: notificationHeading
            anchors {
                fill: parent
                leftMargin: notificationItem.headingLeftPadding
                rightMargin: notificationItem.headingRightPadding
            }

            inGroup: notificationItem.inGroup

            notificationType: notificationItem.notificationType
            jobState: notificationItem.jobState
            jobDetails: notificationItem.jobDetails

            onConfigureClicked: notificationItem.configureClicked()
            onDismissClicked: notificationItem.dismissClicked()
            onCloseClicked: notificationItem.closeClicked()
        }
    }

    RowLayout {
        id: defaultHeaderContainer
        Layout.fillWidth: true
    }

    // Notification body
    RowLayout {
        id: bodyRow
        Layout.fillWidth: true
        spacing: units.smallSpacing

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            RowLayout {
                id: summaryRow
                Layout.fillWidth: true
                visible: summaryLabel.text !== ""

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
                                if (notificationItem.summary) {
                                    return i18ndc("plasma_applet_org.kde.plasma.notifications", "Job name, e.g. Copying is paused", "%1 (Paused)", notificationItem.summary);
                                }
                            } else if (notificationItem.jobState === NotificationManager.Notifications.JobStateStopped) {
                                if (notificationItem.jobError) {
                                    if (notificationItem.summary) {
                                        return i18ndc("plasma_applet_org.kde.plasma.notifications", "Job name, e.g. Copying has failed", "%1 (Failed)", notificationItem.summary);
                                    } else {
                                        return i18nd("plasma_applet_org.kde.plasma.notifications", "Job Failed");
                                    }
                                } else {
                                    if (notificationItem.summary) {
                                        return i18ndc("plasma_applet_org.kde.plasma.notifications", "Job name, e.g. Copying has finished", "%1 (Finished)", notificationItem.summary);
                                    } else {
                                        return i18nd("plasma_applet_org.kde.plasma.notifications", "Job Finished");
                                    }
                                }
                            }
                        }
                        // some apps use their app name as summary, avoid showing the same text twice
                        // try very hard to match the two
                        if (notificationItem.summary && notificationItem.summary.toLocaleLowerCase().trim() != notificationItem.applicationName.toLocaleLowerCase().trim()) {
                            return notificationItem.summary;
                        }
                        return "";
                    }
                    visible: text !== ""
                }

                // inGroup headerItem is reparented here
            }

            RowLayout {
                id: bodyTextRow

                Layout.fillWidth: true
                spacing: units.smallSpacing

                SelectableLabel {
                    id: bodyLabel
                    // FIXME how to assign this via State? target: bodyLabel.Layout doesn't work and just assigning the property doesn't either
                    Layout.alignment: notificationItem.inGroup ? Qt.AlignTop : Qt.AlignVCenter
                    Layout.fillWidth: true

                    Layout.maximumHeight: notificationItem.maximumLineCount > 0
                                          ? (theme.mSize(font).height * notificationItem.maximumLineCount) : -1
                    text: notificationItem.body
                    // Cannot do text !== "" because RichText adds some HTML tags even when empty
                    visible: notificationItem.body !== ""
                    onClicked: notificationItem.bodyClicked(mouse)
                    onLinkActivated: Qt.openUrlExternally(link)
                }

                // inGroup iconContainer is reparented here
            }
        }

        Item {
            id: iconContainer

            Layout.preferredWidth: units.iconSizes.large
            Layout.preferredHeight: units.iconSizes.large
            Layout.topMargin: units.smallSpacing
            Layout.bottomMargin: units.smallSpacing

            visible: iconItem.active || imageItem.active

            PlasmaCore.IconItem {
                id: iconItem
                // don't show two identical icons
                readonly property bool active: valid && source != notificationItem.applicationIconSource
                anchors.fill: parent
                usesPlasmaTheme: false
                smooth: true
                source: {
                    var icon = notificationItem.icon;
                    if (typeof icon !== "string") { // displayed by QImageItem below
                        return "";
                    }

                    // don't show a generic "info" icon since this is a notification already
                    if (icon === "dialog-information") {
                        return "";
                    }

                    return icon;
                }
                visible: active
            }

            KQCAddons.QImageItem {
                id: imageItem
                readonly property bool active: !null && nativeWidth > 0
                anchors.fill: parent
                smooth: true
                fillMode: KQCAddons.QImageItem.PreserveAspectFit
                visible: active
                image: typeof notificationItem.icon === "object" ? notificationItem.icon : undefined
            }

            // JobItem reparents a file icon here for finished jobs with one total file
        }
    }

    // Job progress reporting
    Loader {
        id: jobLoader
        Layout.fillWidth: true
        active: notificationItem.notificationType === NotificationManager.Notifications.JobType
        visible: active
        sourceComponent: JobItem {
            iconContainerItem: iconContainer

            jobState: notificationItem.jobState
            jobError: notificationItem.jobError
            percentage: notificationItem.percentage
            suspendable: notificationItem.suspendable
            killable: notificationItem.killable

            jobDetails: notificationItem.jobDetails
            showDetails: notificationItem.showDetails

            onSuspendJobClicked: notificationItem.suspendJobClicked()
            onResumeJobClicked: notificationItem.resumeJobClicked()
            onKillJobClicked: notificationItem.killJobClicked()

            onOpenUrl: notificationItem.openUrl(url)
            onFileActionInvoked: notificationItem.fileActionInvoked()

            hovered: notificationItem.hovered
        }
    }

    Item {
        id: actionContainer
        Layout.fillWidth: true
        Layout.preferredHeight: Math.max(actionFlow.implicitHeight, replyLoader.height)
        visible: actionRepeater.count > 0

        // Notification actions
        Flow { // it's a Flow so it can wrap if too long
            id: actionFlow
            width: parent.width
            spacing: units.smallSpacing
            layoutDirection: Qt.RightToLeft
            enabled: !replyLoader.active
            opacity: replyLoader.active ? 0 : 1
            Behavior on opacity {
                NumberAnimation {
                    duration: units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }

            Repeater {
                id: actionRepeater

                model: {
                    var buttons = [];
                    var actionNames = (notificationItem.actionNames || []);
                    var actionLabels = (notificationItem.actionLabels || []);
                    // HACK We want the actions to be right-aligned but Flow also reverses
                    for (var i = actionNames.length - 1; i >= 0; --i) {
                        buttons.push({
                            actionName: actionNames[i],
                            label: actionLabels[i]
                        });
                    }

                    if (notificationItem.hasReplyAction) {
                        buttons.unshift({
                            actionName: "inline-reply",
                            label: notificationItem.replyActionLabel || i18nc("Reply to message", "Reply")
                        });
                    }

                    return buttons;
                }

                PlasmaComponents3.ToolButton {
                    flat: false
                    // why does it spit "cannot assign undefined to string" when a notification becomes expired?
                    text: modelData.label || ""

                    onClicked: {
                        if (modelData.actionName === "inline-reply") {
                            replyLoader.beginReply();
                            return;
                        }

                        notificationItem.actionInvoked(modelData.actionName);
                    }
                }
            }
        }

        // inline reply field
        Loader {
            id: replyLoader
            width: parent.width
            height: active ? item.implicitHeight : 0
            // When there is only one action and it is a reply action, show text field right away
            active: notificationItem.replying || (notificationItem.hasReplyAction && (notificationItem.actionNames || []).length === 0)
            visible: active
            opacity: active ? 1 : 0
            x: active ? 0 : parent.width
            Behavior on x {
                NumberAnimation {
                    duration: units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }

            function beginReply() {
                notificationItem.replying = true;

                plasmoid.nativeInterface.forceActivateWindow(notificationItem.Window.window);
                replyLoader.item.activate();
            }

            sourceComponent: NotificationReplyField {
                placeholderText: notificationItem.replyPlaceholderText
                buttonIconName: notificationItem.replySubmitButtonIconName
                buttonText: notificationItem.replySubmitButtonText
                onReplied: notificationItem.replied(text)

                replying: notificationItem.replying
                onBeginReplyRequested: replyLoader.beginReply()
            }
        }
    }

    // thumbnails
    Loader {
        id: thumbnailStripLoader
        Layout.leftMargin: notificationItem.thumbnailLeftPadding
        Layout.rightMargin: notificationItem.thumbnailRightPadding
        // no change in Layout.topMargin to keep spacing to notification text consistent
        Layout.topMargin: 0
        Layout.bottomMargin: notificationItem.thumbnailBottomPadding
        Layout.fillWidth: true
        active: notificationItem.urls.length > 0
        visible: active
        sourceComponent: ThumbnailStrip {
            leftPadding: -thumbnailStripLoader.Layout.leftMargin
            rightPadding: -thumbnailStripLoader.Layout.rightMargin
            topPadding: -notificationItem.thumbnailTopPadding
            bottomPadding: -thumbnailStripLoader.Layout.bottomMargin
            urls: notificationItem.urls
            onOpenUrl: notificationItem.openUrl(url)
            onFileActionInvoked: notificationItem.fileActionInvoked()
        }
    }

    states: [
         State {
            when: notificationItem.inGroup
            PropertyChanges {
                target: headingElement
                parent: summaryRow
            }

            PropertyChanges {
                target: summaryRow
                visible: true
            }
            PropertyChanges {
                target: summaryLabel
                visible: true
            }

            /*PropertyChanges {
                target: bodyLabel.Label
                alignment: Qt.AlignTop
            }*/

            PropertyChanges {
                target: iconContainer
                parent: bodyTextRow
            }
        }
    ]
}
