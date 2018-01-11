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

    property var notificationProperties: ({})
    signal notificationTimeout()

    onVisibleChanged: {
        if (!visible) {
            notificationTimer.stop();
        }
    }

    onYChanged: {
        if (visible && !notificationItem.dragging) {
            notificationTimer.restart();
        }
    }

    function populatePopup(notification) {
        notificationProperties = notification
        notificationTimer.interval = notification.expireTimeout
        notificationTimer.restart();
        //temporarly disable height binding, avoids an useless window resize when removing the old actions
        heightBinding.when = false;
        // notification.actions is a JS array, but we can easily append that to our model
        notificationItem.actions.clear()
        notificationItem.actions.append(notificationProperties.actions)
        //enable height binding again, finally do the resize
        heightBinding.when = true;
    }

    function clearPopup() {
        notificationProperties = {}
        notificationItem.actions.clear()
    }

    mainItem: NotificationItem {
        id: notificationItem
        hoverEnabled: true

        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        //the binding needs to be disabled when re-populating actions, to minimize resizes
        Binding on height {
            id: heightBinding
            value: notificationItem.implicitHeight
            when: true
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
        onContainsMouseChanged: {
            if (containsMouse) {
                notificationTimer.stop()
            } else if (!containsMouse && !dragging && visible) {
                notificationTimer.restart()
            }
        }
        onDraggingChanged: {
            if (dragging) {
                notificationTimer.stop()
            } else if (!containsMouse && !dragging && visible) {
                notificationTimer.restart()
            }
        }

        summary: notificationProperties.summary || ""
        body: notificationProperties.body || ""
        icon: notificationProperties.appIcon || ""
        image: notificationProperties.image
        // explicit true/false or else it complains about assigning undefined to bool
        configurable: notificationProperties.configurable && !Settings.isMobile ? true : false
        urls: notificationProperties.urls || []
        hasDefaultAction: notificationProperties.hasDefaultAction || false
        hasConfigureAction: notificationProperties.hasConfigureAction || false

        width: Math.round(23 * units.gridUnit)
        maximumTextHeight: theme.mSize(theme.defaultFont).height * 10

        onClose: {
            closeNotification(notificationProperties.source)
            // the popup will be closed in response to sourceRemoved
        }
        onConfigure: {
            configureNotification(notificationProperties.appRealName, notificationProperties.eventId)
            notificationPositioner.closePopup(notificationProperties.source);
        }
        onAction: {
            executeAction(notificationProperties.source, actionId)
            actions.clear()
        }
        onOpenUrl: {
            Qt.openUrlExternally(url)
            notificationPositioner.closePopup(notificationProperties.source);
        }
    }
}
