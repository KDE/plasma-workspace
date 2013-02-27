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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0

import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.qtextracomponents 0.1 as QtExtras

import org.kde.draganddrop 1.0 as DragAndDrop

// MousePage

PlasmaComponents.Page {
    id: dragPage
    height: 400
    width: 400
//     anchors {
//         fill: parent
//         margins: _s
//     }
    PlasmaExtras.Title {
        id: dlabel
        text: "Drag & Drop"
        anchors { left: parent.left; right: parent.right; top: parent.top; }
    }
    Item {
        anchors { left: parent.left; right: parent.right; top: dlabel.bottom; bottom: parent.bottom; }

        DragAndDrop.DragArea {
            width: parent.width / 2
            delegate: Rectangle { width: 64; height: 64; color: "yellow"; opacity: 0.6; }
            anchors { left: parent.left; bottom: parent.bottom; top: parent.top; }
            Rectangle { anchors.fill: parent; color: "blue"; opacity: 0.2; }
            onDragStarted: print("started");
            onDrop: print("drop: " + action);
        }

        DragAndDrop.DropArea {
            width: parent.width / 2
            anchors { right: parent.right; bottom: parent.bottom; top: parent.top; }
            Rectangle { anchors.fill: parent; color: "green"; opacity: 0.2; }

            onDrop: print("drop");
            onDragEnter: print("enter");
            onDragLeave: print("leave");
        }

    }
//     QtExtras.MouseEventListener {
//         id: mel
//         hoverEnabled: true
//         anchors { left: parent.left; right: parent.right; top: mellabel.bottom; bottom: parent.bottom; }
//         onPressed: {
//             print("Pressed");
//             melstatus.text = "pressed";
//         }
//         onPositionChanged: {
//             print("positionChanged: " + mouse.x + "," + mouse.y);
//         }
//         onReleased: {
//             print("Released");
//             melstatus.text = "Released";
//         }
//     }
}

