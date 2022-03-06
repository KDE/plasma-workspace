/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.8
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.plasmoid 2.0

import org.kde.quickcharts 1.0 as Charts

import "global"

MouseArea {
    id: compactRoot

    readonly property bool inPanel: (Plasmoid.location === PlasmaCore.Types.TopEdge
        || Plasmoid.location === PlasmaCore.Types.RightEdge
        || Plasmoid.location === PlasmaCore.Types.BottomEdge
        || Plasmoid.location === PlasmaCore.Types.LeftEdge)

    Layout.minimumWidth: Plasmoid.formFactor === PlasmaCore.Types.Horizontal ? height : PlasmaCore.Units.iconSizes.small
    Layout.minimumHeight: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? width : (PlasmaCore.Units.iconSizes.small + 2 * PlasmaCore.Theme.mSize(PlasmaCore.Theme.defaultFont).height)

    Layout.maximumWidth: inPanel ? PlasmaCore.Units.iconSizeHints.panel : -1
    Layout.maximumHeight: inPanel ? PlasmaCore.Units.iconSizeHints.panel : -1

    acceptedButtons: Qt.LeftButton | Qt.MiddleButton

    property int activeCount: 0
    property int unreadCount: 0

    property int jobsCount: 0
    property int jobsPercentage: 0

    property bool inhibited: false

    property bool wasExpanded: false
    onPressed: wasExpanded = Plasmoid.expanded
    onClicked: {
        if (mouse.button === Qt.MiddleButton) {
            Globals.toggleDoNotDisturbMode();
        } else {
            Plasmoid.expanded = !wasExpanded;
        }
    }

    PlasmaCore.Svg {
        id: notificationSvg
        imagePath: "icons/notification"
        colorGroup: PlasmaCore.ColorScope.colorGroup
    }

    PlasmaCore.SvgItem {
        id: notificationIcon
        anchors.centerIn: parent
        width: PlasmaCore.Units.roundToIconSize(Math.min(parent.width, parent.height))
        height: width
        svg: notificationSvg
        visible: opacity > 0

        elementId: "notification-inactive"

        Charts.PieChart {
            id: chart

            anchors.fill: parent

            visible: false

            range { from: 0; to: 100; automatic: false }

            valueSources: Charts.SingleValueSource { value: compactRoot.jobsPercentage }
            colorSource: Charts.SingleValueSource { value: PlasmaCore.Theme.highlightColor }

            thickness: PlasmaCore.Units.devicePixelRatio * 5
        }

        PlasmaComponents3.Label {
            id: countLabel
            anchors.centerIn: parent
            width: Math.round(Math.min(parent.width, parent.height) * (text.length > 1 ? 0.67 : 0.75))
            height: width
            fontSizeMode: Text.Fit
            font.pointSize: 1024
            font.pixelSize: -1
            minimumPointSize: 5//PlasmaCore.Theme.smallestFont.pointSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: compactRoot.unreadCount || ""
            renderType: Text.QtRendering
            visible: false
        }

        PlasmaComponents3.BusyIndicator {
            id: busyIndicator
            anchors.fill: parent
            visible: false
            running: visible
        }
    }

    PlasmaCore.IconItem {
        id: dndIcon
        anchors.fill: parent
        source: "notifications-disabled"
        opacity: 0
        scale: 2
        visible: opacity > 0
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
                visible: true
            }
            PropertyChanges {
                target: busyIndicator
                visible: compactRoot.jobsPercentage == 0
            }
            PropertyChanges {
                target: chart
                visible: true
            }
        },
        State { // do not disturb
            when: compactRoot.inhibited
            PropertyChanges {
                target: dndIcon
                scale: 1
                opacity: 1
            }
            PropertyChanges {
                target: notificationIcon
                scale: 0
                opacity: 0
            }
        },
        State { // unread notifications
            name: "UNREAD"
            when: compactRoot.unreadCount > 0
            PropertyChanges {
                target: notificationIcon
                elementId: "notification-active"
            }
        }
    ]

    transitions: [
        Transition {
            to: "*" // any state
            NumberAnimation {
                targets: [notificationIcon, dndIcon]
                properties: "opacity,scale"
                duration: PlasmaCore.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        },
        Transition {
            from: ""
            to: "UNREAD"
            SequentialAnimation {
                RotationAnimation {
                    target: notificationIcon
                    to: 30
                    easing.type: Easing.InOutQuad
                    duration: PlasmaCore.Units.longDuration
                }
                RotationAnimation {
                    target: notificationIcon
                    to: -30
                    easing.type: Easing.InOutQuad
                    duration: PlasmaCore.Units.longDuration * 2 // twice the swing distance, keep speed uniform
                }
                RotationAnimation {
                    target: notificationIcon
                    to: 0
                    easing.type: Easing.InOutQuad
                    duration: PlasmaCore.Units.longDuration
                }
            }
        }
    ]

}
