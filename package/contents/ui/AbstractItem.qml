/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaCore.ToolTipArea {
    id: abstractItem

    height: hidden ? root.hiddenItemSize : root.itemSize
    width: labelVisible ? parent.width : height

    property alias text: label.text
    property bool hidden: parent.objectName == "hiddenTasksColumn"
    property bool labelVisible: abstractItem.hidden && !root.activeApplet
    property Item iconItem

    signal clicked(var mouse)
    signal wheel(var wheel)



    /* subclasses need to assign to this tiiltip properties
    mainText:
    subText:
    icon: 
    */


    location: if (abstractItem.parent && abstractItem.parent.objectName == "hiddenTasksColumn") {
                return PlasmaCore.Types.RightEdge;
              } else {
                return abstractItem.location;
              }


    //onIconItemChanged: iconItem.parent = mouseArea

    MouseArea {
        id: mouseArea
        anchors.fill: abstractItem
        hoverEnabled: true
        drag.filterChildren: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onEntered: {
            if (hidden) {
                root.hiddenLayout.hoveredItem = abstractItem
            }
        }
        onClicked: abstractItem.clicked(mouse)
        onWheel: abstractItem.wheel(wheel)
    }

    PlasmaComponents.Label {
        id: label
        opacity: labelVisible ? 1 : 0
        x: iconItem ? iconItem.width + units.smallSpacing : 0
        Behavior on opacity {
            NumberAnimation {
                duration: units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
        anchors {
            verticalCenter: parent.verticalCenter
        }
    }
}

