/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
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
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.configuration 2.0
import org.kde.qtextracomponents 2.0 as QtExtras

PlasmaComponents.ToolButton {
    flat: false
    text: "Screen edge"

    QtExtras.MouseEventListener {
        anchors.fill: parent
        property int lastX
        property int lastY
        property int startMouseX
        property int startMouseY
        onPressed: {
            lastX = mouse.screenX
            lastY = mouse.screenY
            startMouseX = mouse.x
            startMouseY = mouse.y
        }
        onPositionChanged: {
            switch (panel.location) {
            //TopEdge
            case 3:
                configDialog.y = mouse.screenY - mapToItem(dialogRoot, 0, startMouseY).y
                panel.y = configDialog.y - panel.height
                break
            //LeftEdge
            case 5:
                configDialog.x = mouse.screenX - mapToItem(dialogRoot, startMouseX, 0).x
                panel.x = configDialog.x - panel.width
                break;
            //RightEdge
            case 6:
                configDialog.x = mouse.screenX - mapToItem(dialogRoot, startMouseX, 0).x
                panel.x = configDialog.x + configDialog.width
                break;
            //BottomEdge
            case 4:
            default:
                configDialog.y = mouse.screenY - mapToItem(dialogRoot, 0, startMouseY).y
                panel.y = configDialog.y + configDialog.height
            }

            lastX = mouse.screenX
            lastY = mouse.screenY

            var screenAspect = panel.screenGeometry.height / panel.screenGeometry.width
            var newLocation = panel.location

            //If the mouse is in an internal rectangle, do nothing
            if ((mouse.screenX > panel.screenGeometry.x + panel.screenGeometry.width/3 &&
                 mouse.screenX < panel.screenGeometry.x + panel.screenGeometry.width/3*2) &&
                (mouse.screenY > panel.screenGeometry.y + panel.screenGeometry.height/3 &&
                 mouse.screenY < panel.screenGeometry.y + panel.screenGeometry.height/3*2)) {
                return;
            }


            if (mouse.screenY < panel.screenGeometry.y+(mouse.screenX-panel.screenGeometry.x)*screenAspect) {
                if (mouse.screenY < panel.screenGeometry.y + panel.screenGeometry.height-(mouse.screenX-panel.screenGeometry.x)*screenAspect) {
                    if (panel.location == 3) {
                        return;
                    } else {
                        newLocation = PlasmaCore.Types.TopEdge;
                    }
                } else if (panel.location == 6) {
                        return;
                } else {
                    newLocation = PlasmaCore.Types.RightEdge;
                }

            } else {
                if (mouse.screenY < panel.screenGeometry.y + panel.screenGeometry.height-(mouse.screenX-panel.screenGeometry.x)*screenAspect) {
                    if (panel.location == 5) {
                        return;
                    } else {
                        newLocation = PlasmaCore.Types.LeftEdge;
                    }
                } else if(panel.location == 4) {
                return;
                } else {
                    newLocation = PlasmaCore.Types.BottomEdge;
                }
            }
            panel.location = newLocation
            print("New Location: " + newLocation);
        }
        onReleased: {
            panelResetAnimation.running = true
        }
    }
}
