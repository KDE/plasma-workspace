/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.qtextracomponents 2.0 as QtExtras

// MousePage

PlasmaComponents.Page {
    id: mousePage
    anchors {
        fill: parent
        margins: _s
    }
    PlasmaExtras.Title {
        id: mellabel
        text: "MouseEventListener"
        anchors { left: parent.left; right: parent.right; top: parent.top }
    }
    QtExtras.MouseEventListener {
        id: mel
        hoverEnabled: true
        anchors { left: parent.left; right: parent.right; top: mellabel.bottom; bottom: parent.bottom; }
        /*
        void pressed(KDeclarativeMouseEvent *mouse);
        void positionChanged(KDeclarativeMouseEvent *mouse);
        void released(KDeclarativeMouseEvent *mouse);
        void clicked(KDeclarativeMouseEvent *mouse);
        void pressAndHold(KDeclarativeMouseEvent *mouse);
        void wheelMoved(KDeclarativeWheelEvent *wheel);
        void containsMouseChanged(bool containsMouseChanged);
        void hoverEnabledChanged(bool hoverEnabled);
        */
        onPressed: {
            print("Pressed");
            melstatus.text = "pressed";
        }
        onPositionChanged: {
            print("positionChanged: " + mouse.x + "," + mouse.y);
        }
        onReleased: {
            print("Released");
            melstatus.text = "Released";
        }
        onPressAndHold: {
            print("pressAndHold");
            melstatus.text = "pressAndHold";
        }
        onClicked: {
            print("Clicked");
            melstatus.text = "clicked";
        }
        onWheelMoved: {
            print("Wheel: " + wheel.delta);
        }
        onContainsMouseChanged: {
            print("Contains mouse: " + containsMouse);
        }

        MouseArea {
            //target: mel
            anchors.fill: parent
            onPressed: PlasmaExtras.DisappearAnimation { targetItem: bgImage }
            onReleased: PlasmaExtras.AppearAnimation { targetItem: bgImage }
        }
        Image {
            id: bgImage
            source: "image://appbackgrounds/standard"
            fillMode: Image.Tile
            anchors.fill: parent
            asynchronous: true
//                 opacity: mel.containsMouse ? 1 : 0.2
//                 Behavior on opacity { PropertyAnimation {} }
        }
        Column {
            //width: parent.width
            spacing: _s
            anchors.fill: parent
            PlasmaComponents.Button {
                text: "Button"
                iconSource: "call-start"
            }
            PlasmaComponents.ToolButton {
                text: "ToolButton"
                iconSource: "call-stop"
            }
            PlasmaComponents.RadioButton {
                text: "RadioButton"
                //iconSource: "call-stop"
            }
            PlasmaComponents.Label {
                id: melstatus
            }
        }

    }
}

