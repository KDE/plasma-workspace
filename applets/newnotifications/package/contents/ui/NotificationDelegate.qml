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

import org.kde.plasma.core 2.0 as PlasmaCore

ColumnLayout {
    id: delegate

    property alias notificationType: notificationItem.notificationType

    property alias headerVisible: notificationItem.headerVisible

    property alias applicationName: notificationItem.applicationName
    property alias applicatonIconSource: notificationItem.applicationIconSource
    property alias deviceName: notificationItem.deviceName

    property alias time: notificationItem.time

    property alias summary: notificationItem.summary
    property alias body: notificationItem.body
    property alias icon: notificationItem.icon
    property alias urls: notificationItem.urls

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

    property alias actionNames: notificationItem.actionNames
    property alias actionLabels: notificationItem.actionLabels

    signal configureClicked
    signal dismissClicked
    signal closeClicked

    //signal defaultActionInvoked
    signal actionInvoked(string actionName)
    signal openUrl(string url)

    signal suspendJobClicked
    signal resumeJobClicked
    signal killJobClicked

    // FIXME
    property alias svg: lineSvgItem.svg

    spacing: 0

    NotificationItem {
        id: notificationItem
        Layout.fillWidth: true

        closable: true

        onCloseClicked: delegate.closeClicked()
        onDismissClicked: delegate.dismissClicked()
        onConfigureClicked: delegate.configureClicked()

        onActionInvoked: delegate.actionInvoked(actionName)
        onOpenUrl: delegate.openUrl(url)

        onSuspendJobClicked: delegate.suspendJobClicked()
        onResumeJobClicked: delegate.resumeJobClicked()
        onKillJobClicked: delegate.killJobClicked()
    }

    PlasmaCore.SvgItem {
        id: lineSvgItem
        elementId: "horizontal-line"
        Layout.fillWidth: true
        // TODO hide for last notification
    }
}
