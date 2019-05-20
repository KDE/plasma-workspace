/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
 *   Copyright 2019 ivan tkachenko <ratijastk@kde.org>
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

import QtQuick 2.1
import QtQuick.Layouts 1.12
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaCore.ToolTipArea {
    id: abstractItem
    objectName: "abstractItem"

    Layout.fillHeight: !(labelVisible || vertical)
    Layout.fillWidth:    labelVisible || vertical
    Layout.preferredHeight: effectiveItemSize + marginHints.top + marginHints.bottom
    Layout.preferredWidth: labelVisible ? -1 : effectiveItemSize + marginHints.left + marginHints.right

    property real effectiveItemSize: hidden ? root.hiddenItemSize : root.itemSize
    property string itemId
    property string category
    property alias text: label.text
    property bool hidden: parent.objectName === "hiddenTasksColumn"
    property QtObject marginHints: parent.marginHints
    property alias labelVisible: label.visible
    property Item iconItemContainer: iconItemContainer
    property Item iconItem
    property int /* PlasmaCore.Types.ItemStatus */ status
    property QtObject model

    signal clicked(var mouse)
    signal wheel(var wheel)
    signal contextMenu(var mouse)

    property bool forcedHidden: plasmoid.configuration.hiddenItems.indexOf(itemId) !== -1
    property bool forcedShown: plasmoid.configuration.showAllItems || plasmoid.configuration.shownItems.indexOf(itemId) !== -1
    property bool categoryShown: shownCategories.indexOf(category) !== -1;

    readonly property int effectiveStatus: {
        if (!categoryShown || status === PlasmaCore.Types.HiddenStatus) {
            return PlasmaCore.Types.HiddenStatus
        } else if (forcedShown || (!forcedHidden && status !== PlasmaCore.Types.PassiveStatus)) {
            return PlasmaCore.Types.ActiveStatus
        } else {
            return PlasmaCore.Types.PassiveStatus
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            id: iconItemContainer
            property real size: Math.min(parent.width - marginHints.left - marginHints.right,
                                         parent.height - marginHints.top - marginHints.bottom)
            Layout.preferredWidth: abstractItem.effectiveItemSize
            Layout.preferredHeight: abstractItem.effectiveItemSize
            Layout.leftMargin: labelVisible ? marginHints.left : 0
            Layout.rightMargin: labelVisible ? marginHints.right : 0
            Layout.alignment: Qt.AlignVCenter | (labelVisible ? Qt.AlignLeft : Qt.AlignHCenter)
        }
        PlasmaComponents.Label {
            id: label
            Layout.fillWidth: true

            opacity: visible ? 1 : 0
            visible: abstractItem.hidden && !root.activeApplet
            Behavior on opacity {
                NumberAnimation {
                    duration: units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }

    /* subclasses need to assign to this tooltip properties
    mainText:
    subText:
    icon:
    */

    location: {
        if (hidden) {
            if (LayoutMirroring.enabled && plasmoid.location !== PlasmaCore.Types.RightEdge) {
                return PlasmaCore.Types.LeftEdge;
            } else if (plasmoid.location !== PlasmaCore.Types.LeftEdge) {
                return PlasmaCore.Types.RightEdge;
            }
        }

        return plasmoid.location;
    }

//BEGIN CONNECTIONS

    onEffectiveStatusChanged: updateItemVisibility(abstractItem);

    onContainsMouseChanged: {
        if (hidden && containsMouse) {
            root.hiddenLayout.hoveredItem = abstractItem
        }
    }

    Component.onCompleted: updateItemVisibility(abstractItem);

    //dangerous but needed due how repeater reparents
    onParentChanged: updateItemVisibility(abstractItem);

    onIconItemChanged: {
        if (iconItem) {
            iconItem.parent = iconItemContainer;
            iconItem.anchors.fill = iconItemContainer;
        }
    }

//END CONNECTIONS

    PulseAnimation {
        targetItem: iconItem
        running: (abstractItem.status === PlasmaCore.Types.NeedsAttentionStatus ||
            abstractItem.status === PlasmaCore.Types.RequiresAttentionStatus ) &&
            units.longDuration > 0
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        drag.filterChildren: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        onClicked: abstractItem.clicked(mouse)
        onPressed: {
            abstractItem.hideToolTip()
            if (mouse.button === Qt.RightButton) {
                abstractItem.contextMenu(mouse)
            }
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
}

