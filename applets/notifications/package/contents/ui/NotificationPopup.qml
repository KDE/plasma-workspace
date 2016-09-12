/*
 *   Copyright 2014 Martin Klapetek <mklapetek@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Controls.Private 1.0
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

PlasmaCore.Dialog {
    id: notificationPopup

    location: PlasmaCore.Types.Floating
    type: PlasmaCore.Dialog.Notification
    flags: Qt.WindowDoesNotAcceptFocus

    property var notificationProperties
    signal notificationTimeout()

    onVisibleChanged: {
        if (!visible) {
            notificationTimer.stop();
        }
    }

    onYChanged: {
        if (visible) {
            notificationTimer.restart();
        }
    }

    function populatePopup(notification) {
        notificationProperties = notification
        notificationTimer.interval = notification.expireTimeout
        notificationTimer.restart()

        // notification.actions is a JS array, but we can easily append that to our model
        notificationItem.actions.clear()
        notificationItem.actions.append(notificationProperties.actions)
    }

    Behavior on y {
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.OutQuad
        }
    }

    mainItem: KQuickControlsAddons.MouseEventListener {
        id: root
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        width: notificationItem.width + 2 * notificationItem.x
        height: notificationItem.implicitHeight + 2 * notificationItem.y

        hoverEnabled: true

        onClicked: {
            // the MEL would close the notification before the action button
            // onClicked handler would fire effectively breaking notification actions
            if (notificationItem.pressedAction()) {
                return
            }

            closeNotification(notificationProperties.source)
            notificationPopup.hide()
        }
        onContainsMouseChanged: {
            if (containsMouse) {
                notificationTimer.stop()
            } else if (!containsMouse && visible) {
                notificationTimer.restart()
            }
        }

        Timer {
            id: notificationTimer
            onTriggered: {
                if (!notificationProperties.isPersistent) {
                    expireNotification(notificationProperties.source)
                }
                notificationPopup.notificationTimeout();
            }
        }

        NotificationItem {
            id: notificationItem

            summary: notificationProperties ? notificationProperties.summary: ""
            body: notificationProperties ? notificationProperties.body : ""
            icon: notificationProperties ? notificationProperties.appIcon : ""
            image: notificationProperties ? notificationProperties.image : undefined
            configurable: (notificationProperties ? notificationProperties.configurable : false) && !Settings.isMobile

            x: units.smallSpacing
            y: units.smallSpacing

            width: Math.round(23 * units.gridUnit)
            maximumTextHeight: theme.mSize(theme.defaultFont).height * 10

            onClose: {
                closeNotification(notificationProperties.source)
                notificationPopup.hide()
            }
            onConfigure: {
                configureNotification(notificationProperties.appRealName, notificationProperties.eventId)
                notificationPopup.hide()
            }
            onAction: {
                executeAction(notificationProperties.source, actionId)
                actions.clear()
                notificationPopup.hide()
            }
        }
    }

}
