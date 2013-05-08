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
import "panelconfiguration"


//TODO: all of this will be done with desktop components
PlasmaCore.FrameSvgItem {
    id: root

    imagePath: "dialogs/background"

    state: {
        switch (panel.location) {
        //TopEdge
        case 3:
            return "TopEdge"
        //LeftEdge
        case 5:
            return "LeftEdge"
        //RightEdge
        case 6:
            return "RightEdge"
        //BottomEdge
        case 4:
        default:
            return "BottomEdge"
        }
    }

//BEGIN properties
    width: 640
    height: 64
//END properties

//BEGIN Connections
    Connections {
        target: panel
        onOffsetChanged: ruler.offset = panel.offset
        onMinimumLengthChanged: ruler.minimumLength = panel.minimumLength
        onMaximumLengthChanged: ruler.maximumLength = panel.maximumLength
    }
//END Connections


//BEGIN UI components

    Ruler {
        id: ruler
        state: root.state
    }

    PlasmaComponents.ButtonRow {
        spacing: 0
        exclusive: false
        anchors {
            centerIn: parent
        }
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
                    print("New Location: " + newLocation);
                }
                onReleased: panelResetAnimation.running = true
            }
        }
        PlasmaComponents.ToolButton {
            flat: false
            text: "Height"
            QtExtras.MouseEventListener {
                anchors.fill: parent
                property int startMouseX
                property int startMouseY
                onPressed: {
                    startMouseX = mouse.x
                    startMouseY = mouse.y
                }
                onPositionChanged: {
                    switch (panel.location) {
                    //TopEdge
                    case 3:
                        configDialog.y = mouse.screenY - mapToItem(root, 0, startMouseY).y
                        panel.thickness = configDialog.y - panel.y
                        break;
                    //LeftEdge
                    case 5:
                        configDialog.x = mouse.screenX - mapToItem(root, startMouseX, 0).x
                        panel.thickness = configDialog.x - panel.x
                        break;
                    //RightEdge
                    case 6:
                        configDialog.x = mouse.screenX - mapToItem(root, startMouseX, 0).x
                        panel.thickness = (panel.x + panel.width) - (configDialog.x + configDialog.width)
                        break;
                    //BottomEdge
                    case 4:
                    default:
                        configDialog.y = mouse.screenY - mapToItem(root, 0, startMouseY).y
                        panel.thickness = (panel.y + panel.height) - (configDialog.y + configDialog.height)
                    }
                }
            }
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
                    return panel.screenGeometry.x + panel.screenGeometry.width - panel.width
                    break;
                //BottomEdge
                case 4:
                default:
                    return panel.screenGeometry.y + panel.screenGeometry.height - panel.height
                }
            }
            duration: 150
        }
        NumberAnimation {
            target: configDialog
            properties: (panel.location == 5 || panel.location == 6) ? "x" : "y"
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
                    return panel.screenGeometry.x + panel.screenGeometry.width - panel.width - configDialog.width
                    break;
                //BottomEdge
                case 4:
                default:
                    return panel.screenGeometry.y + panel.screenGeometry.height - panel.height - configDialog.height
                }
            }
            duration: 150
        }
    }
//END UI components

//BEGIN States
states: [
        State {
            name: "TopEdge"
            PropertyChanges {
                target: root
                enabledBorders: "TopBorder|BottomBorder"
            }
        },
        State {
            name: "BottomEdge"
            PropertyChanges {
                target: root
                enabledBorders: "TopBorder|BottomBorder"
            }
        },
        State {
            name: "LeftEdge"
            PropertyChanges {
                target: root
                enabledBorders: "LeftBorder|RightBorder"
            }
        },
        State {
            name: "RightEdge"
            PropertyChanges {
                target: root
                enabledBorders: "LeftBorder|RightBorder"
            }
        }
    ]
//END States
}
