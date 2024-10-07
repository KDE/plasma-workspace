/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.notificationmanager as NotificationManager

QtObject {
    id: root

    property int maximumLineCount: 0
    property int bodyCursorShape: Qt.ArrowCursor

    property int notificationType

    property bool inGroup: false
    property bool inHistory: false
    //TODO: REMOVE
    property ListView listViewParent: null

    property var applicationIconSource
    property string applicationName
    property string originName

    property string summary
    property var time

    property bool configurable
    property bool dismissable
    property bool dismissed
    property bool closable

    // This isn't an alias because TextEdit RichText adds some HTML tags to it
    property string body
    property string accessibleDescription
    property var icon
    property var urls: []
    property int urgency: NotificationManager.Notifications.NormalUrgency

    property int jobState
    property int percentage
    property int jobError: 0
    property bool suspendable
    property bool killable

    property QtObject jobDetails

    property string configureActionLabel
    property var actionNames: []
    property var actionLabels: []
    property bool hasDefaultAction

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

    property real timeout
    property real remainingTime

    readonly property real textPreferredWidth: Kirigami.Units.gridUnit * 18

    signal bodyClicked
    signal closeClicked
    signal configureClicked
    signal dismissClicked
    signal actionInvoked(string actionName)
    signal defaultActionInvoked
    signal replied(string text)
    signal openUrl(string url)
    signal fileActionInvoked(QtObject action)
    signal forceActiveFocusRequested

    signal suspendJobClicked
    signal resumeJobClicked
    signal killJobClicked
}
