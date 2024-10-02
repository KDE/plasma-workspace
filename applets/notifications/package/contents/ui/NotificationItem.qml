/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Window

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami


import org.kde.notificationmanager as NotificationManager

import org.kde.plasma.private.notifications 2.0 as Notifications

import "global"

import "delegates" as Delegates

ColumnLayout {
    id: notificationItem

    // We don't want the popups to grow too much due to very long labels
    Layout.preferredWidth: Math.max(footerLoader.implicitWidth, Globals.popupWidth)
    Layout.preferredHeight: implicitHeight

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
    property string accessibleDescription
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

    property int headingLeftMargin: 0
    property int headingRightMargin: 0
    property int headingLeftPadding: 0
    property int headingRightPadding: 0

    property int thumbnailLeftPadding: 0
    property int thumbnailRightPadding: 0
    property int thumbnailTopPadding: 0
    property int thumbnailBottomPadding: 0

    property alias timeout: notificationHeading.timeout
    property alias remainingTime: notificationHeading.remainingTime

    readonly property bool menuOpen: bodyLabel.contextMenu !== null
                                     || Boolean(footerLoader.item?.menuOpen)

    readonly property bool dragging: Boolean(footerLoader.item?.dragging)
    readonly property bool replying: footerLoader.item?.replying ?? false
    readonly property bool hasPendingReply: footerLoader.item?.hasPendingReply ?? false
    readonly property alias headerHeight: headingElement.height
    readonly property real textPreferredWidth: Kirigami.Units.gridUnit * 18

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

    spacing: Kirigami.Units.smallSpacing

    // Header
    RowLayout {
        id: headingElement

        visible: !notificationItem.inGroup
        Layout.fillWidth: true
        Layout.leftMargin: notificationItem.headingLeftMargin
        Layout.rightMargin: notificationItem.headingRightMargin

        Kirigami.Theme.colorSet: Kirigami.Theme.Header
        Kirigami.Theme.inherit: false


        PlasmaExtras.PlasmoidHeading {
            id: heading
            topInset: 0
            Layout.fillWidth: true
            background.visible: !notificationItem.inHistory
            parent: notificationItem.inGroup ? summaryRow : headingElement
            leftPadding: headingLeftPadding
            rightPadding: headingRightPadding
            bottomPadding: 0

            // HACK PlasmoidHeading is a QQC2 Control which accepts left mouse button by default,
            // which breaks the popup default action mouse handler, cf. QTBUG-89785
            Component.onCompleted: Notifications.InputDisabler.makeTransparentForInput(this)

            contentItem: Delegates.NotificationHeader {
                id: notificationHeading

                inGroup: notificationItem.inGroup
                inHistory: notificationItem.inHistory

                notificationType: notificationItem.notificationType
                jobState: notificationItem.jobState
                jobDetails: notificationItem.jobDetails

                onConfigureClicked: notificationItem.configureClicked()
                onDismissClicked: notificationItem.dismissClicked()
                onCloseClicked: notificationItem.closeClicked()
            }
        }
    }

    // Everything else that goes below the header
    // This is its own ColumnLayout-within-a-ColumnLayout because it lets us set
    // the left margin once rather than several times, in each of its children
    GridLayout {
        Layout.fillWidth: true

        rowSpacing: notificationItem.spacing
        columnSpacing: notificationItem.spacing

        Accessible.role: notificationItem.inHistory ? Accessible.NoRole : Accessible.Notification
        Accessible.name: summaryLabel.text
        Accessible.description: notificationItem.accessibleDescription
        columns: 2
        visible: !notificationItem.inGroup

        LayoutItemProxy {
            Layout.fillWidth: true
            Layout.preferredWidth: Math.min(textPreferredWidth, summaryRow.implicitWidth)
            target: summaryRow
        }
        LayoutItemProxy {
            Layout.rowSpan: 2
            target: iconContainer
        }
        LayoutItemProxy {
            Layout.fillWidth: true
            target: bodyLabel
        }
    }

    GridLayout {
        Layout.fillWidth: true

        rowSpacing: notificationItem.spacing
        columnSpacing: notificationItem.spacing

        Accessible.role: notificationItem.inHistory ? Accessible.NoRole : Accessible.Notification
        Accessible.name: summaryLabel.text
        Accessible.description: notificationItem.accessibleDescription
        visible: notificationItem.inGroup
        columns: 2

        LayoutItemProxy {
            target: summaryRow
            Layout.fillWidth: true
            Layout.columnSpan: 2
        }
        LayoutItemProxy {
            Layout.fillWidth: true
            target: bodyLabel
        }
        LayoutItemProxy { target: iconContainer }
    }


    // Notification body
    RowLayout {
        id: summaryRow

        Layout.alignment: Qt.AlignTop
        visible: summaryLabel.text !== ""

        Delegates.Heading {
            id: summaryLabel
            summary: notificationItem.summary
            notificationType: notificationItem.notificationType
            jobState: notificationItem.jobState
            applicationName: notificationItem.applicationName
            Layout.topMargin: notificationItem.inGroup && lineCount > 1 ? Math.max(0, (headingElement.Layout.preferredHeight - summaryLabelTextMetrics.height) / 2) : 0
        }

        // inGroup headerItem is reparented here
    }

    Delegates.Body {
        id: bodyLabel
        maximumLineCount: notificationItem.maximumLineCount
        body: notificationItem.body
        onClicked: notificationItem.bodyClicked()
    }

    Delegates.Icon {
        id: iconContainer

        source: notificationItem.icon
        applicationIconSource: notificationItem.applicationIconSource
    }

    Loader {
        id: footerLoader
        Layout.fillWidth: true
        visible: active
        sourceComponent: {
            if (notificationItem.notificationType === NotificationManager.Notifications.JobType) {
                return jobComponent;
            } else if (notificationItem.urls.length > 0) {
                return thumbnailComponent
            } else if (notificationItem.actionNames.length > 0) {
                return actionComponent;
            }
            return undefined;
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
        }
    ]


    // Actions
    Component {
        id: actionComponent
        Delegates.ActionContainer {
            id: actionContainer
            Layout.fillWidth: true

            actionNames: notificationItem.actionNames
            actionLabels: notificationItem.actionLabels

            hasReplyAction: notificationItem.hasReplyAction
            //replying: notificationItem.replying
            replyActionLabel: notificationItem.replyActionLabel
            replyPlaceholderText: notificationItem.replyPlaceholderText
            replySubmitButtonIconName: notificationItem.replySubmitButtonIconName
            replySubmitButtonText: notificationItem.replySubmitButtonText

            onForceActiveFocusRequested: notificationItem.forceActiveFocusRequested()
            onActionInvoked: actionName => notificationItem.actionInvoked(actionName)
            onReplied: text => notificationItem.replied(text)
        }
    }

    // Jobs
    Component {
        id: jobComponent
        Delegates.JobItem {
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

    // Thumbnails (contains actions as well)
    Component {
        id: thumbnailComponent
        Delegates.ThumbnailStrip {
            leftPadding: notificationItem.thumbnailLeftPadding
            rightPadding: notificationItem.thumbnailRightPadding
            topPadding: notificationItem.thumbnailTopPadding
            bottomPadding: notificationItem.thumbnailBottomPadding
            urls: notificationItem.urls
            onOpenUrl: notificationItem.openUrl(url)
            onFileActionInvoked: notificationItem.fileActionInvoked(action)

            actionNames: notificationItem.actionNames
            actionLabels: notificationItem.actionLabels

            hasReplyAction: notificationItem.hasReplyAction
            //replying: notificationItem.replying
            replyActionLabel: notificationItem.replyActionLabel
            replyPlaceholderText: notificationItem.replyPlaceholderText
            replySubmitButtonIconName: notificationItem.replySubmitButtonIconName
            replySubmitButtonText: notificationItem.replySubmitButtonText

            onForceActiveFocusRequested: notificationItem.forceActiveFocusRequested()
            onActionInvoked: actionName => notificationItem.actionInvoked(actionName)
            onReplied: text => notificationItem.replied(text)
        }
    }
}
