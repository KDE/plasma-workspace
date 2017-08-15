/***************************************************************************
 *   Copyright 2011 Davide Bettio <davide.bettio@kdemail.net>              *
 *   Copyright 2011 Marco Martin <mart@kde.org>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0

Item {
    PlasmaCore.SvgItem {
        id: notificationSvgItem
        anchors.centerIn: parent
        width: units.roundToIconSize(Math.min(parent.width, parent.height))
        height: width

        svg: notificationSvg

        elementId: {
            if (activeItemsCount > 0) {
                if (jobs && jobs.count > 0) {
                    return "notification-progress-inactive"
                } else {
                    return "notification-empty"
                }
            }
            return "notification-disabled"
        }

        state: notificationsApplet.state

        PlasmaCore.Svg {
            id: notificationSvg
            imagePath: "icons/notification"
            colorGroup: PlasmaCore.ColorScope.colorGroup
        }

        Item {
            id: jobProgressItem
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            width: notificationSvgItem.width * globalProgress

            clip: true
            visible: jobs.count > 0

            PlasmaCore.SvgItem {
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                }
                width: notificationSvgItem.width

                svg: notificationSvg
                elementId: "notification-progress-active"
            }
        }

        PlasmaComponents.BusyIndicator {
            anchors.fill: parent

            visible: jobs ? jobs.count > 0 : false
            running: visible
        }

        PlasmaComponents.Label {
            id: notificationCountLabel
            property int oldActiveItemsCount: 0

            // anchors.fill: parent breaks at small sizes for some reason
            anchors.centerIn: parent
            width: parent.width - (units.smallSpacing * 2.5 * text.length)
            height: width

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: notificationsApplet.activeItemsCount
            font.pointSize: 100
            fontSizeMode: Text.Fit
            minimumPointSize: theme.smallestFont.pointSize
            visible: notificationsApplet.activeItemsCount > 0

            Connections {
                target: notificationsApplet
                onActiveItemsCountChanged: {
                    if (notificationsApplet.activeItemsCount > notificationCountLabel.oldActiveItemsCount) {
                        notificationAnimation.running = true
                    }
                    notificationCountLabel.oldActiveItemsCount = notificationsApplet.activeItemsCount
                }

            }
        }

        PlasmaCore.SvgItem {
            id: notificationAnimatedItem
            anchors.fill: parent

            svg: notificationSvg
            elementId: "notification-active"
            opacity: 0
            scale: 2

            SequentialAnimation {
                id: notificationAnimation

                NumberAnimation {
                    target: notificationAnimatedItem
                    duration: units.longDuration
                    properties: "opacity, scale"
                    to: 1
                    easing.type: Easing.InOutQuad
                }

                PauseAnimation { duration: units.longDuration * 2 }

                ParallelAnimation {
                    NumberAnimation {
                        target: notificationAnimatedItem
                        duration: units.longDuration
                        properties: "opacity"
                        to: 0
                        easing.type: Easing.InOutQuad
                    }

                    NumberAnimation {
                        target: notificationAnimatedItem
                        duration: units.longDuration
                        properties: "scale"
                        to: 2
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            property bool wasExpanded: false

            onPressed: wasExpanded = plasmoid.expanded
            onClicked: plasmoid.expanded = !wasExpanded
        }
    }
}
