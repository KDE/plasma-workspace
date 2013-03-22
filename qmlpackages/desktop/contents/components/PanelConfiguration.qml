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

//TODO: all of this will be done with desktop components
Rectangle {
    id: root

//BEGIN properties
    color: "lightgray"
    width: 640
    height: 64
//END properties

//BEGIN UI components
    Rectangle {
        width: 32
        height: 32
        MouseArea {
            drag {
                target: parent
                axis: (panel.location == 5 || panel.location == 6) ? Drag.YAxis : Drag.XAxis
            }
            anchors.fill: parent
            onPositionChanged: {
                if (panel.location == 5 || panel.location == 6) {
                    panel.offset = parent.y
                } else {
                    panel.offset = parent.x
                }
            }
            Component.onCompleted: {
                if (panel.location == 5 || panel.location == 6) {
                    panel.offset = parent.y
                } else {
                    panel.offset = parent.x
                }
            }
        }
    }

    Rectangle {
        width: 100
        height: 32
        anchors {
            centerIn: parent
        }
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
                    configDialog.y = mouse.screenY - mapToItem(root, 0, startMouseY).y
                    panel.y = configDialog.y - panel.height
                    break
                //LeftEdge
                case 5:
                    configDialog.x = mouse.screenX - mapToItem(root, startMouseX, 0).x
                    panel.x = configDialog.x - panel.width
                    break;
                //RightEdge
                case 6:
                    configDialog.x = mouse.screenX - mapToItem(root, startMouseX, 0).x
                    panel.x = configDialog.x + configDialog.width
                    break;
                //BottomEdge
                case 4:
                default:
                    configDialog.y = mouse.screenY - mapToItem(root, 0, startMouseY).y
                    panel.y = configDialog.y + configDialog.height
                }

                lastX = mouse.screenX
                lastY = mouse.screenY

                var screenAspect = panel.screenGeometry.height / panel.screenGeometry.width
                var newLocation = panel.location

                if (mouse.screenY < panel.screenGeometry.y+(mouse.screenX-panel.screenGeometry.x)*screenAspect) {
                    if (mouse.screenY < panel.screenGeometry.y + panel.screenGeometry.height-(mouse.screenX-panel.screenGeometry.x)*screenAspect) {
                        if (panel.location == 3) {
                            return;
                        } else {
                            newLocation = 3; //FIXME: Plasma::TopEdge;
                        }
                    } else if (panel.location == 6) {
                            return;
                    } else {
                        newLocation = 6; //FIXME: Plasma::RightEdge;
                    }

                } else {
                    if (mouse.screenY < panel.screenGeometry.y + panel.screenGeometry.height-(mouse.screenX-panel.screenGeometry.x)*screenAspect) {
                        if (panel.location == 5) {
                            return;
                        } else {
                            newLocation = 5; //FIXME: Plasma::LeftEdge;
                        }
                    } else if(panel.location == 4) {
                            return;
                    } else {
                        newLocation = 4; //FIXME: Plasma::BottomEdge;
                    }
                }
                panel.location = newLocation
                if (panel.location == 5 || panel.location == 6) {
                    configDialog.y = panel.screenGeometry.y
                    root.width = 100
                    root.height = panel.screenGeometry.height
                } else {
                    configDialog.x = panel.screenGeometry.x
                    root.height = 100
                    root.width = panel.screenGeometry.width
                } 
                print("New Location: " + newLocation);
            }
            onReleased: panelResetAnimation.running = true
        }
    }
    ParallelAnimation {
        id: panelResetAnimation
        NumberAnimation {
            target: panel
            properties: (panel.location == 5 || panel.location == 6) ? "x" : "y"
            to:  {
                switch (panel.location) {
                //TopEdge
                case 3:
                    return 0
                    break
                //LeftEdge
                case 5:
                    return 0
                    break;
                //RightEdge
                case 6:
                    return panel.screenGeometry.y + panel.screenGeometry.height - panel.height
                    break;
                //BottomEdge
                case 4:
                default:
                    return panel.screenGeometry.x + panel.screenGeometry.width - panel.width
                }
            }
            duration: 150
        }
        NumberAnimation {
            target: configDialog
            properties: "y"
            to: {
            panel.height
                switch (panel.location) {
                //TopEdge
                case 3:
                    return panel.height
                    break
                //LeftEdge
                case 5:
                    return panel.width
                    break;
                //RightEdge
                case 6:
                    return panel.x - configDialog.width
                    break;
                //BottomEdge
                case 4:
                default:
                    return panel.y - configDialog.height
                }
            }
            duration: 150
        }
    }
//END UI components
}
