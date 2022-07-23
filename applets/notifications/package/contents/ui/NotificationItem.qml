/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.2

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kquickcontrolsaddons 2.0 as KQCAddons

import org.kde.notificationmanager 1.0 as NotificationManager

ColumnLayout {
    id: notificationItem

    property int maximumLineCount: 0
    property alias bodyCursorShape: bodyLabel.cursorShape

    property int notificationType

    property bool inGroup: false
    property bool inHistory: false
    property ListView listViewParent: null

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
    readonly property bool hasPendingReply: replyLoader.item && replyLoader.item.text !== ""
    readonly property alias headerHeight: headingElement.height
    property int extraSpaceForCriticalNotificationLine: 0

    signal bodyClicked
    signal closeClicked
    signal configureClicked
    signal dismissClicked
    signal actionInvoked(string actionName)
    signal replied(string text)
    signal openUrl(string url)
    signal fileActionInvoked(QtObject action)
    signal forceActiveFocusRequested

    signal suspendJobClicked
    signal resumeJobClicked
    signal killJobClicked

    spacing: PlasmaCore.Units.smallSpacing

    // Header
    Item {
        id: headingElement
        Layout.fillWidth: true
        Layout.preferredHeight: notificationHeading.implicitHeight
        Layout.preferredWidth: notificationHeading.implicitWidth
        Layout.bottomMargin: -parent.spacing

        PlasmaCore.ColorScope.colorGroup: PlasmaCore.Theme.HeaderColorGroup
        PlasmaCore.ColorScope.inherit: false

        PlasmaExtras.PlasmoidHeading {
            anchors.fill: parent
            visible: !notificationItem.inHistory
        }

        NotificationHeader {
            id: notificationHeading
            anchors {
                fill: parent
                leftMargin: notificationItem.headingLeftPadding
                rightMargin: notificationItem.headingRightPadding
            }

            PlasmaCore.ColorScope.colorGroup: parent.PlasmaCore.ColorScope.colorGroup
            PlasmaCore.ColorScope.inherit: false

            inGroup: notificationItem.inGroup

            notificationType: notificationItem.notificationType
            jobState: notificationItem.jobState
            jobDetails: notificationItem.jobDetails

            onConfigureClicked: notificationItem.configureClicked()
            onDismissClicked: notificationItem.dismissClicked()
            onCloseClicked: notificationItem.closeClicked()
        }
    }

    // Everything else that goes below the header
    // This is its own ColumnLayout-within-a-ColumnLayout because it lets us set
    // the left margin once rather than several times, in each of its children
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: notificationItem.extraSpaceForCriticalNotificationLine
        spacing: PlasmaCore.Units.smallSpacing

        // Notification body
        RowLayout {
            id: bodyRow
            Layout.fillWidth: true

            spacing: PlasmaCore.Units.smallSpacing

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: summaryRow.implicitHeight + bodyTextRow.implicitHeight

                activeFocusOnTab: true

                Accessible.name: summaryLabel.text
                Accessible.description: bodyLabel.textItem.getText(0, notificationItem.body.length)
                Accessible.role: Accessible.Notification
                Keys.forwardTo: bodyLabel.textItem

                Loader {
                    anchors.fill: parent
                    active: parent.activeFocus
                    asynchronous: true
                    z: -1

                    sourceComponent: PlasmaExtras.Highlight {
                        hovered: true
                    }
                }

                RowLayout {
                    id: summaryRow

                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                    }
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
                        // Give it a bit more visual prominence than the app name in the header
                        type: PlasmaExtras.Heading.Type.Primary
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

                    anchors {
                        left: parent.left
                        right: parent.right
                        top: summaryRow.bottom
                    }
                    spacing: PlasmaCore.Units.smallSpacing

                    SelectableLabel {
                        id: bodyLabel
                        listViewParent: notificationItem.listViewParent
                        // FIXME how to assign this via State? target: bodyLabel.Layout doesn't work and just assigning the property doesn't either
                        Layout.alignment: notificationItem.inGroup ? Qt.AlignTop : Qt.AlignVCenter
                        Layout.fillWidth: true

                        Layout.maximumHeight: notificationItem.maximumLineCount > 0
                                            ? (theme.mSize(font).height * notificationItem.maximumLineCount) : -1

                        // HACK RichText does not allow to specify link color and since LineEdit
                        // does not support StyledText, we have to inject some CSS to force the color,
                        // cf. QTBUG-81463 and to some extent QTBUG-80354
                        text: "<style>a { color: " + PlasmaCore.Theme.linkColor + "; }</style>" + notificationItem.body

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

                Layout.preferredWidth: PlasmaCore.Units.iconSizes.large
                Layout.preferredHeight: PlasmaCore.Units.iconSizes.large
                Layout.topMargin: PlasmaCore.Units.smallSpacing
                Layout.bottomMargin: PlasmaCore.Units.smallSpacing

                visible: iconItem.active

                PlasmaCore.IconItem {
                    id: iconItem
                    // don't show two identical icons
                    readonly property bool active: valid && source != notificationItem.applicationIconSource
                    anchors.fill: parent
                    usesPlasmaTheme: false
                    smooth: true
                    // don't show a generic "info" icon since this is a notification already
                    source: notificationItem.icon !== "dialog-information" ? notificationItem.icon : ""
                    visible: active
                }

                // JobItem reparents a file icon here for finished jobs with one total file
            }
        }

        // Job progress reporting
        Loader {
            id: jobLoader
            Layout.fillWidth: true
            active: notificationItem.notificationType === NotificationManager.Notifications.JobType
            height: item ? item.implicitHeight : 0
            visible: active
            sourceComponent: JobItem {
                iconContainerItem: iconContainer

                jobState: notificationItem.jobState
                jobError: notificationItem.jobError
                percentage: notificationItem.percentage
                suspendable: notificationItem.suspendable
                killable: notificationItem.killable

                jobDetails: notificationItem.jobDetails

                onSuspendJobClicked: notificationItem.suspendJobClicked()
                onResumeJobClicked: notificationItem.resumeJobClicked()
                onKillJobClicked: notificationItem.killJobClicked()

                onOpenUrl: notificationItem.openUrl(url)
                onFileActionInvoked: notificationItem.fileActionInvoked(action)
            }
        }

        // Actions
        Item {
            id: actionContainer
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(actionFlow.implicitHeight, replyLoader.height)
            visible: actionRepeater.count > 0 && actionFlow.parent === this

            // Notification actions
            Flow { // it's a Flow so it can wrap if too long
                id: actionFlow
                // For a cleaner look, if there is a thumbnail, puts the actions next to the thumbnail strip's menu button
                parent: thumbnailStripLoader.item ? thumbnailStripLoader.item.actionContainer : actionContainer
                width: parent.width
                spacing: PlasmaCore.Units.smallSpacing
                layoutDirection: Qt.RightToLeft
                enabled: !replyLoader.active
                opacity: replyLoader.active ? 0 : 1
                Behavior on opacity {
                    NumberAnimation {
                        duration: PlasmaCore.Units.longDuration
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
                        duration: PlasmaCore.Units.longDuration
                        easing.type: Easing.InOutQuad
                    }
                }
                Behavior on opacity {
                    NumberAnimation {
                        duration: PlasmaCore.Units.longDuration
                        easing.type: Easing.InOutQuad
                    }
                }

                function beginReply() {
                    notificationItem.replying = true;

                    notificationItem.forceActiveFocusRequested();
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
                onFileActionInvoked: notificationItem.fileActionInvoked(action)
            }
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
