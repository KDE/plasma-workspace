/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

import org.kde.plasma.core 2.0 as PlasmaCore

import org.kde.notificationmanager 1.0 as NotificationManager

import ".."

PlasmaCore.Dialog {
    id: notificationPopup

    property int popupWidth

    property alias notificationType: notificationItem.notificationType

    property alias applicationName: notificationItem.applicationName
    property alias applicationIconSource: notificationItem.applicationIconSource
    property alias originName: notificationItem.originName

    property alias time: notificationItem.time

    property alias summary: notificationItem.summary
    property alias body: notificationItem.body
    property alias icon: notificationItem.icon
    property alias urls: notificationItem.urls

    property int urgency
    property int timeout
    property int dismissTimeout

    property alias jobState: notificationItem.jobState
    property alias percentage: notificationItem.percentage
    property alias jobError: notificationItem.jobError
    property alias suspendable: notificationItem.suspendable
    property alias killable: notificationItem.killable
    property alias jobDetails: notificationItem.jobDetails

    property alias configureActionLabel: notificationItem.configureActionLabel
    property alias configurable: notificationItem.configurable
    property alias dismissable: notificationItem.dismissable
    property alias closable: notificationItem.closable

    property bool hasDefaultAction
    property var defaultActionFallbackWindowIdx
    property alias actionNames: notificationItem.actionNames
    property alias actionLabels: notificationItem.actionLabels

    property alias hasReplyAction: notificationItem.hasReplyAction
    property alias replyActionLabel: notificationItem.replyActionLabel
    property alias replyPlaceholderText: notificationItem.replyPlaceholderText
    property alias replySubmitButtonText: notificationItem.replySubmitButtonText
    property alias replySubmitButtonIconName: notificationItem.replySubmitButtonIconName

    signal configureClicked
    signal dismissClicked
    signal closeClicked

    signal defaultActionInvoked
    signal actionInvoked(string actionName)
    signal replied(string text)
    signal openUrl(string url)
    signal fileActionInvoked

    signal expired
    signal hoverEntered
    signal hoverExited

    signal suspendJobClicked
    signal resumeJobClicked
    signal killJobClicked

    property int defaultTimeout: 5000
    readonly property int effectiveTimeout: {
        if (timeout === -1) {
            return defaultTimeout;
        }
        if (dismissTimeout) {
            return dismissTimeout;
        }
        return timeout;
    }

    location: PlasmaCore.Types.Floating
    flags: notificationItem.replying ? 0 : Qt.WindowDoesNotAcceptFocus

    visible: false

    // When notification is updated, restart hide timer
    onTimeChanged: {
        if (timer.running) {
            timer.restart();
        }
    }

    mainItem: Item {
        width: notificationPopup.popupWidth
        height: notificationItem.implicitHeight + notificationItem.y
        DraggableDelegate {
            id: area
            width: parent.width
            height: parent.height
            hoverEnabled: true
            draggable: notificationItem.notificationType != NotificationManager.Notifications.JobType
            onDismissRequested: popupNotificationsModel.close(popupNotificationsModel.index(index, 0))

            cursorShape: hasDefaultAction ? Qt.PointingHandCursor : Qt.ArrowCursor
            acceptedButtons: hasDefaultAction || draggable ? Qt.LeftButton : Qt.NoButton

            onClicked: {
                if (hasDefaultAction) {
                    notificationPopup.defaultActionInvoked();
                }
            }
            onEntered: notificationPopup.hoverEntered()
            onExited: notificationPopup.hoverExited()

            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true

            Timer {
                id: timer
                interval: notificationPopup.effectiveTimeout
                running: notificationPopup.visible && !area.containsMouse && interval > 0
                    && !notificationItem.dragging && !notificationItem.menuOpen && !notificationItem.replying
                onTriggered: {
                    if (notificationPopup.dismissTimeout) {
                        notificationPopup.dismissClicked();
                    } else {
                        notificationPopup.expired();
                    }
                }
            }

            NumberAnimation {
                target: notificationItem
                property: "remainingTime"
                from: timer.interval
                to: 0
                duration: timer.interval
                running: timer.running && units.longDuration > 1
            }

            NotificationItem {
                id: notificationItem
                // let the item bleed into the dialog margins so the close button margins cancel out
                y: closable || dismissable || configurable ? -notificationPopup.margins.top : 0
                headingRightPadding: -notificationPopup.margins.right
                width: parent.width
                hovered: area.containsMouse
                maximumLineCount: 8
                bodyCursorShape: notificationPopup.hasDefaultAction ? Qt.PointingHandCursor : 0

                thumbnailLeftPadding: -notificationPopup.margins.left
                thumbnailRightPadding: -notificationPopup.margins.right
                thumbnailTopPadding: -notificationPopup.margins.top
                thumbnailBottomPadding: -notificationPopup.margins.bottom

                timeout: timer.running ? timer.interval : 0

                closable: true
                onBodyClicked: {
                    if (area.acceptedButtons & mouse.button) {
                        area.clicked(null /*mouse*/);
                    }
                }
                onCloseClicked: notificationPopup.closeClicked()
                onDismissClicked: notificationPopup.dismissClicked()
                onConfigureClicked: notificationPopup.configureClicked()
                onActionInvoked: notificationPopup.actionInvoked(actionName)
                onReplied: notificationPopup.replied(text)
                onOpenUrl: notificationPopup.openUrl(url)
                onFileActionInvoked: notificationPopup.fileActionInvoked()

                onSuspendJobClicked: notificationPopup.suspendJobClicked()
                onResumeJobClicked: notificationPopup.resumeJobClicked()
                onKillJobClicked: notificationPopup.killJobClicked()
            }
        }
    }
}
