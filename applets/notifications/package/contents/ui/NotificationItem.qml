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

ColumnLayout {
    id: notificationItem

    // We don't want the popups to grow too much due to very long labels
    Layout.preferredWidth: Math.max(actionRow.implicitWidth, Globals.popupWidth - Kirigami.Units.smallSpacing * 2)
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
                                     || Boolean(thumbnailStripLoader.item?.menuOpen || jobLoader.item?.menuOpen)

    readonly property bool dragging: Boolean(thumbnailStripLoader.item?.dragging || jobLoader.item?.dragging)
    property bool replying: false
    readonly property bool hasPendingReply: replyLoader.item?.text !== ""
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

            contentItem: NotificationHeader {
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
        Component.onCompleted: {
            // Workaround for https://bugreports.qt.io/browse/QTBUG-126196
            // remove as soon as we can depend from a fixed Qt
            // Invoking this undocumented slot will force the layout
            // to be reevaluated and all items resized again
            invalidateSenderItem()
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

        Kirigami.Heading {
            id: summaryLabel
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            Layout.topMargin: notificationItem.inGroup && lineCount > 1 ? Math.max(0, (headingElement.Layout.preferredHeight - summaryLabelTextMetrics.height) / 2) : 0
            textFormat: Text.PlainText
            maximumLineCount: 3
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            level: 4
            // Give it a bit more visual prominence than the app name in the header
            type: Kirigami.Heading.Type.Primary
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

            TextMetrics {
                id: summaryLabelTextMetrics
                font: summaryLabel.font
                text: summaryLabel.text
            }
        }

        // inGroup headerItem is reparented here
    }

    SelectableLabel {
        id: bodyLabel

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        readonly property real maximumHeight: Kirigami.Units.gridUnit * notificationItem.maximumLineCount
        readonly property bool truncated: notificationItem.maximumLineCount > 0 && bodyLabel.implicitHeight > maximumHeight
        Layout.maximumHeight: truncated ? maximumHeight : implicitHeight

        listViewParent: notificationItem.listViewParent
        // HACK RichText does not allow to specify link color and since LineEdit
        // does not support StyledText, we have to inject some CSS to force the color,
        // cf. QTBUG-81463 and to some extent QTBUG-80354
        text: "<style>a { color: " + Kirigami.Theme.linkColor + "; }</style>" + notificationItem.body

        // Cannot do text !== "" because RichText adds some HTML tags even when empty
        visible: notificationItem.body !== ""
        onClicked: notificationItem.bodyClicked()
        onLinkActivated: Qt.openUrlExternally(link)
    }

    Item {
        id: iconContainer

        Layout.alignment: Qt.AlignTop
        implicitWidth: visible ? iconItem.width : 0
        implicitHeight: visible ? Math.max(iconItem.height, bodyLabel.height + (notificationItem.inGroup ? 0 : summaryRow.implicitHeight)) : 0

        visible: iconItem.shouldBeShown

        Kirigami.Icon {
            id: iconItem

            width: Kirigami.Units.iconSizes.large
            height: Kirigami.Units.iconSizes.large
            anchors.verticalCenter: parent.verticalCenter

            // don't show two identical icons
            readonly property bool shouldBeShown: valid && source != notificationItem.applicationIconSource

            smooth: true
            source: notificationItem.icon
        }
    }

    // Job progress reporting
    Loader {
        id: jobLoader
        Layout.fillWidth: true
        Layout.preferredHeight: item ? item.implicitHeight : 0
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
        Layout.preferredHeight: childrenRect.height
        visible: actionRepeater.count > 0 && actionRow.parent === this

        implicitWidth: actionRow.implicitWidth
        implicitHeight: actionRow.implicitHeight

        // Notification actions
        RowLayout {
            id: actionRow
            // For a cleaner look, if there is a thumbnail, puts the actions next to the thumbnail strip's menu button
            parent: thumbnailStripLoader.item?.actionContainer ?? actionContainer
            width: parent.width
            spacing: Kirigami.Units.smallSpacing

            enabled: !replyLoader.active
            opacity: replyLoader.active ? 0 : 1
            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // If the actions are large enough to be the thing that determines
                // the popup size, we should hide it as otherwise we end up with a
                // `spacing` sized gap on the leading side of the row.
                visible: actionRow.implicitWidth < notificationItem.width
            }

            Repeater {
                id: actionRepeater

                model: {
                    var buttons = [];
                    var actionNames = (notificationItem.actionNames || []);
                    var actionLabels = (notificationItem.actionLabels || []);

                    for (var i = 0; i < actionNames.length; ++i) {
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
                    Layout.fillWidth: true
                    Layout.maximumWidth: implicitWidth

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
            height: active && item ? item.implicitHeight : 0
            // When there is only one action and it is a reply action, show text field right away
            active: notificationItem.replying || (notificationItem.hasReplyAction && (notificationItem.actionNames || []).length === 0)
            visible: active
            opacity: active ? 1 : 0
            x: active ? 0 : parent.width
            Behavior on x {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
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

    // Thumbnails
    Loader {
        id: thumbnailStripLoader
        Layout.leftMargin: notificationItem.thumbnailLeftPadding
        Layout.rightMargin: notificationItem.thumbnailRightPadding
        // no change in Layout.topMargin to keep spacing to notification text consistent
        Layout.topMargin: 0
        Layout.bottomMargin: notificationItem.thumbnailBottomPadding
        Layout.fillWidth: true
        Layout.preferredHeight: item ? item.implicitHeight : 0
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
}
