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

    mainItem: MouseArea {
        id: root
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        height: notificationItem.implicitHeight + (units.smallSpacing * 2)
        width: notificationItem.width + (units.smallSpacing * 2)

        hoverEnabled: true

        onClicked: {
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
            property string text

            summary: notificationProperties ? notificationProperties.summary: ""
            text: notificationProperties ? notificationProperties.body : ""
            icon: notificationProperties ? notificationProperties.appIcon : ""
            image: notificationProperties ? notificationProperties.image : undefined
            configurable: (notificationProperties ? notificationProperties.configurable : false) && !Settings.isMobile

            x: units.smallSpacing
            y: units.smallSpacing

            width: Math.round(23 * units.gridUnit)

            textItem: PlasmaComponents.Label {
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                elide: Text.ElideRight
                verticalAlignment: Text.AlignTop
                onLinkActivated: Qt.openUrlExternally(link)
                text: notificationItem.text
                textFormat: Text.StyledText
                maximumLineCount: 4
            }

            onClose: {
                closeNotification(notificationProperties.source)
                notificationPopup.hide()
            }
            onConfigure: {
                configureNotification(notificationProperties.appRealName)
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
