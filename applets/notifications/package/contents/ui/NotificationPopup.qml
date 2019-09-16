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
    property alias actionNames: notificationItem.actionNames
    property alias actionLabels: notificationItem.actionLabels

    signal configureClicked
    signal dismissClicked
    signal closeClicked

    signal defaultActionInvoked
    signal actionInvoked(string actionName)
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
        width: notificationPopup.popupWidth
        height: notificationItem.implicitHeight + notificationItem.y
        hoverEnabled: true

        cursorShape: hasDefaultAction ? Qt.PointingHandCursor : Qt.ArrowCursor
        acceptedButtons: hasDefaultAction ? Qt.LeftButton : Qt.NoButton

        onClicked: notificationPopup.defaultActionInvoked()
        onEntered: notificationPopup.hoverEntered()
        onExited: notificationPopup.hoverExited()

        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        Timer {
            id: timer
            interval: notificationPopup.effectiveTimeout
            running: notificationPopup.visible && !area.containsMouse && interval > 0
                && !notificationItem.dragging && !notificationItem.menuOpen
            onTriggered: {
                if (notificationPopup.dismissTimeout) {
                    notificationPopup.dismissClicked();
                } else {
                    notificationPopup.expired();
                }
            }
        }

        Timer {
            id: timeoutIndicatorDelayTimer
            // only show indicator for the last ten seconds of timeout
            readonly property int remainingTimeout: 10000
            interval: Math.max(0, timer.interval - remainingTimeout)
            running: interval > 0 && timer.running
        }

        Rectangle {
            id: timeoutIndicatorRect
            anchors {
                right: parent.right
                rightMargin: -notificationPopup.margins.right
                bottom: parent.bottom
                bottomMargin: -notificationPopup.margins.bottom
            }
            width: units.devicePixelRatio * 3
            color: theme.highlightColor
            opacity: timeoutIndicatorAnimation.running ? 0.6 : 0
            visible: units.longDuration > 1
            Behavior on opacity {
                NumberAnimation {
                    duration: units.longDuration
                }
            }

            NumberAnimation {
                id: timeoutIndicatorAnimation
                target: timeoutIndicatorRect
                property: "height"
                from: area.height + notificationPopup.margins.top + notificationPopup.margins.bottom
                to: 0
                duration: Math.min(timer.interval, timeoutIndicatorDelayTimer.remainingTimeout)
                running: timer.running && !timeoutIndicatorDelayTimer.running && units.longDuration > 1
            }
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
            onOpenUrl: notificationPopup.openUrl(url)
            onFileActionInvoked: notificationPopup.fileActionInvoked()

            onSuspendJobClicked: notificationPopup.suspendJobClicked()
            onResumeJobClicked: notificationPopup.resumeJobClicked()
            onKillJobClicked: notificationPopup.killJobClicked()
        }
    }
}
