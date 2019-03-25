/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
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
import org.kde.plasma.components 2.0 as PlasmaComponents

MouseArea {
    id: compactRoot

    // FIXME figure out a way how to let the compact icon not grow beond iconSizeHints
    // but still let it expand eventually for a sidebar

    /*readonly property bool inPanel: (plasmoid.location === PlasmaCore.Types.TopEdge
        || plasmoid.location === PlasmaCore.Types.RightEdge
        || plasmoid.location === PlasmaCore.Types.BottomEdge
        || plasmoid.location === PlasmaCore.Types.LeftEdge)

    Layout.minimumWidth: plasmoid.formFactor === PlasmaCore.Types.Horizontal ? height : units.iconSizes.small
    Layout.minimumHeight: plasmoid.formFactor === PlasmaCore.Types.Vertical ? width : (units.iconSizes.small + 2 * theme.mSize(theme.defaultFont).height)

    Layout.maximumWidth: -1//inPanel ? units.iconSizeHints.panel : -1
    Layout.maximumHeight: inPanel ? units.iconSizeHints.panel : -1*/

    property int activeCount: 0
    property int expiredCount: 0

    property int jobsCount: 0
    property int jobsPercentage: 0

    property bool wasExpanded: false
    onPressed: wasExpanded = plasmoid.expanded
    onClicked: plasmoid.expanded = !wasExpanded

    PlasmaCore.Svg {
        id: notificationSvg
        imagePath: "icons/notification"
        colorGroup: PlasmaCore.ColorScope.colorGroup
    }

    PlasmaCore.SvgItem {
        id: notificationIcon
        anchors.centerIn: parent
        width: units.roundToIconSize(Math.min(parent.width, parent.height))
        height: width
        svg: notificationSvg

        elementId: "notification-disabled"

        Item {
            id: jobProgressItem
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            width: notificationIcon.width * (jobsPercentage / 100)

            clip: true
            visible: false

            PlasmaCore.SvgItem {
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                }
                width: notificationIcon.width

                svg: notificationSvg
                elementId: "notification-progress-active"
            }
        }

        PlasmaComponents.Label {
            id: countLabel
            anchors {
                fill: parent
                margins: units.devicePixelRatio * 2
            }
            height: undefined
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pointSize: 100
            fontSizeMode: Text.Fit
            minimumPointSize: 7
        }

        PlasmaComponents.BusyIndicator {
            id: busyIndicator
            anchors.fill: parent
            visible: false
            running: visible
        }
    }

    states: [
        State { // active process
            when: compactRoot.jobsCount > 0
            PropertyChanges {
                target: notificationIcon
                elementId: "notification-progress-inactive"
            }
            PropertyChanges {
                target: countLabel
                text: compactRoot.jobsCount
            }
            PropertyChanges {
                target: busyIndicator
                visible: true
            }
            PropertyChanges {
                target: jobProgressItem
                visible: true
            }
        },
        State { // active notification
            when: compactRoot.activeCount > 0
            PropertyChanges {
                target: notificationIcon
                elementId: "notification-active";
            }
        },
        State { // unread notifications
            when: compactRoot.expiredCount > 0
            PropertyChanges {
                target: notificationIcon
                elementId: "notification-empty"
            }
            PropertyChanges {
                target: countLabel
                text: compactRoot.expiredCount
            }
        }
    ]

}
