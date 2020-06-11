/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
 *   Copyright 2020 Konrad Materka <materka@gmail.com>
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

import QtQuick 2.2
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaCore.ToolTipArea {
    id: abstractItem

    height: inVisibleLayout ? visibleLayout.cellHeight : hiddenLayout.iconItemHeight
    width: inVisibleLayout ? visibleLayout.cellWidth : hiddenLayout.width

    property var model: itemModel

    property string itemId
    property alias text: label.text
    property alias iconContainer: iconContainer
    property int /*PlasmaCore.Types.ItemStatus*/ status: model.status || PlasmaCore.Types.UnknownStatus
    property int /*PlasmaCore.Types.ItemStatus*/ effectiveStatus: model.effectiveStatus || PlasmaCore.Types.UnknownStatus
    readonly property bool inHiddenLayout: effectiveStatus === PlasmaCore.Types.PassiveStatus
    readonly property bool inVisibleLayout: effectiveStatus === PlasmaCore.Types.ActiveStatus

    signal clicked(var mouse)
    signal pressed(var mouse)
    signal wheel(var wheel)
    signal contextMenu(var mouse)

    /* subclasses need to assign to this tooltip properties
    mainText:
    subText:
    */

    location: {
        if (inHiddenLayout) {
            if (LayoutMirroring.enabled && plasmoid.location !== PlasmaCore.Types.RightEdge) {
                return PlasmaCore.Types.LeftEdge;
            } else if (plasmoid.location !== PlasmaCore.Types.LeftEdge) {
                return PlasmaCore.Types.RightEdge;
            }
        }

        return plasmoid.location;
    }

//BEGIN CONNECTIONS

    onContainsMouseChanged: {
        if (inHiddenLayout && containsMouse) {
            root.hiddenLayout.currentIndex = index
        }
    }

//END CONNECTIONS

    PulseAnimation {
        targetItem: iconContainer
        running: (abstractItem.status === PlasmaCore.Types.NeedsAttentionStatus ||
            abstractItem.status === PlasmaCore.Types.RequiresAttentionStatus ) &&
            units.longDuration > 0
    }

    function activated() {
        activatedAnimation.start()
    }

    SequentialAnimation {
        id: activatedAnimation
        loops: 1

        ScaleAnimator {
            target: iconContainer
            from: 1
            to: 0.5
            duration: units.shortDuration
            easing.type: Easing.InQuad
        }

        ScaleAnimator {
            target: iconContainer
            from: 0.5
            to: 1
            duration: units.shortDuration
            easing.type: Easing.OutQuad
        }
    }

    MouseArea {
        anchors.fill: abstractItem
        hoverEnabled: true
        drag.filterChildren: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        onClicked: abstractItem.clicked(mouse)
        onPressed: {
            abstractItem.hideToolTip()
            abstractItem.pressed(mouse)
        }
        onPressAndHold: {
            abstractItem.contextMenu(mouse)
        }
        onWheel: {
            abstractItem.wheel(wheel);
            //Don't accept the event in order to make the scrolling by mouse wheel working
            //for the parent scrollview this icon is in.
            wheel.accepted = false;
        }
    }

    Row {
        spacing: units.smallSpacing
        anchors.horizontalCenter: inVisibleLayout ? parent.horizontalCenter : undefined
        anchors.verticalCenter: parent.verticalCenter
        Item {
            id: iconContainer
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(abstractItem.width, abstractItem.height)
            height: width
            property alias inHiddenLayout: abstractItem.inHiddenLayout
            property alias inVisibleLayout: abstractItem.inVisibleLayout
        }
        PlasmaComponents.Label {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            visible: abstractItem.inHiddenLayout && !root.activeApplet
            opacity: visible ? 1 : 0
            Behavior on opacity {
                NumberAnimation {
                    duration: units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
}

