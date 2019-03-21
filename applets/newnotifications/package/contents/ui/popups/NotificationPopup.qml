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
import org.kde.plasma.components 2.0 as Components

import org.kde.notificationmanager 1.0 as NotificationManager

import ".."

PlasmaCore.Dialog {
    id: notificationPopup

    property alias notificationType: notificationItem.notificationType
    readonly property bool isNotification: notificationType === NotificationManager.Notifications.NotificationType
    readonly property bool isJob: notificationType === NotificationManager.Notifications.JobType

    property alias applicationName: notificationItem.applicationName
    property alias applicatonIconSource: notificationItem.applicationIconSource

    property alias time: notificationItem.time

    property alias summary: notificationItem.summary
    property alias body: notificationItem.body
    property alias icon: notificationItem.icon
    property alias urls: notificationItem.urls
    property int urgency
    property int timeout

    property alias jobState: notificationItem.jobState
    property alias percentage: notificationItem.percentage
    property alias error: notificationItem.error
    property alias errorText: notificationItem.errorText
    property alias suspendable: notificationItem.suspendable
    property alias killable: notificationItem.killable
    property alias jobDetails: notificationItem.jobDetails

    property alias configureActionLabel: notificationItem.configureActionLabel
    property alias configurable: notificationItem.configurable
    property alias dismissable: notificationItem.dismissable
    property alias closable: notificationItem.closable

    property bool hasDefaultAction
    property alias actionNames: notificationItem.actionNames
    property alias actionLabels: notificationItem.actionLabels

    signal configureClicked
    signal dismissClicked
    signal closeClicked

    signal defaultActionInvoked
    signal actionInvoked(string actionName)
    signal openUrl(string url)
    signal expired

    signal suspendJobClicked
    signal resumeJobClicked
    signal killJobClicked

    // TODO configurable and/or based on notification size
    property int defaultTimeout: 5000
    readonly property int effectiveTimeout: timeout === -1 ? defaultTimeout : timeout

    location: PlasmaCore.Types.Floating
    // FIXME make KWin allow notificaton + ontop hint
    type: urgency === NotificationManager.Notifications.CriticalUrgency ? PlasmaCore.Dialog.OnScreenDisplay
                                                                        : PlasmaCore.Dialog.Notification
    flags: Qt.WindowDoesNotAcceptFocus

    visible: false

    // When notification is updated, restart hide timer
    onTimeChanged: {
        if (timer.running) {
            timer.restart();
        }
    }

    mainItem: MouseArea {
        id: area
        width: popupHandler.popupWidth
        height: notificationItem.implicitHeight
        hoverEnabled: true

        cursorShape: hasDefaultAction ? Qt.PointingHandCursor : Qt.ArrowCursor
        acceptedButtons: hasDefaultAction ? Qt.LeftButton : Qt.NoButton

        onClicked: notificationPopup.defaultActionInvoked()

        Timer {
            id: timer
            interval: notificationPopup.effectiveTimeout
            // FIXME don't run when context menu is open or dragging thumbnail off
            running: notificationPopup.visible && !area.containsMouse && interval > 0
            onTriggered: notificationPopup.expired()
        }

        Rectangle {
            id: timeoutRect
            anchors {
                right: parent.right
                rightMargin: -notificationPopup.margins.right
                bottom: parent.bottom
                bottomMargin: -notificationPopup.margins.bottom
            }
            width: units.devicePixelRatio * 3
            radius: width
            color: theme.highlightColor
            opacity: timer.running ? 0.6 : 0
            visible: units.longDuration > 1
            Behavior on opacity {
                NumberAnimation {
                    duration: units.longDuration
                }
            }

            NumberAnimation {
                target: timeoutRect
                property: "height"
                from: area.height + notificationPopup.margins.top + notificationPopup.margins.bottom
                to: 0
                duration: timer.interval
                running: timer.running && units.longDuration > 1
            }
        }

        NotificationItem {
            id: notificationItem
            width: parent.width
            hovered: area.containsMouse
            maximumLineCount: 8 // TODO configurable?

            thumbnailLeftPadding: -notificationPopup.margins.left
            thumbnailRightPadding: -notificationPopup.margins.right
            thumbnailTopPadding: -notificationPopup.margins.top
            thumbnailBottomPadding: -notificationPopup.margins.bottom

            closable: true // TODO with grouping and what not
            onBodyClicked: {
                if (area.acceptedButtons & mouse.button) {
                    area.clicked(null /*mouse*/)
                }
            }
            onCloseClicked: notificationPopup.closeClicked()
            onDismissClicked: notificationPopup.dismissClicked()
            onConfigureClicked: notificationPopup.configureClicked()
            onActionInvoked: notificationPopup.actionInvoked(actionName)
            onOpenUrl: notificationPopup.openUrl(url)

            onSuspendJobClicked: notificationPopup.suspendJobClicked()
            onResumeJobClicked: notificationPopup.resumeJobClicked()
            onKillJobClicked: notificationPopup.killJobClicked()
        }
    }
}
