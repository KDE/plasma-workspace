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


PlasmaCore.FrameSvgItem {
    id: root

    property alias offset: offsetHandle.value
    property alias minimumLength: minimumLengthHandle.value
    property alias maximumLength: maximumLengthHandle.value

    imagePath: "widgets/containment-controls"
    state: "BottomEdge"
    implicitWidth: offsetHandle.width + minimumLengthHandle.width
    implicitHeight: offsetHandle.height + minimumLengthHandle.height

    Component.onCompleted: {
        offsetHandle.value = panel.offset
        minimumLengthHandle.value = panel.minimumLength
        maximumLengthHandle.value = panel.maximumLength
    }

    PlasmaCore.Svg {
        id: containmentControlsSvg
        imagePath: "widgets/containment-controls"
    }

    SliderHandle {
        id: offsetHandle
        graphicElementName: "offsetslider"
        onValueChanged: panel.offset = value
    }
    SliderHandle {
        id: minimumLengthHandle
        offset: panel.offset
        graphicElementName: "minslider"
        onValueChanged: panel.minimumLength = value
    }
    SliderHandle {
        id: maximumLengthHandle
        offset: panel.offset
        graphicElementName: "maxslider"
        onValueChanged: panel.maximumLength = value
    }

    states: [
        State {
            name: "TopEdge"
            PropertyChanges {
                target: root
                prefix: "north"
                height: root.implicitHeight
            }
            AnchorChanges {
                target: root
                anchors {
                    top: root.parent.top
                    bottom: undefined
                    left: root.parent.left
                    right: root.parent.right
                }
            }
            AnchorChanges {
                target: offsetHandle
                anchors {
                    top: undefined
                    bottom: root.bottom
                    left: undefined
                    right: undefined
                }
            }
            AnchorChanges {
                target: minimumLengthHandle
                anchors {
                    top: root.top
                    bottom: undefined
                    left: undefined
                    right: undefined
                }
            }
            AnchorChanges {
                target: maximumLengthHandle
                anchors {
                    top: undefined
                    bottom: root.bottom
                    left: undefined
                    right: undefined
                }
            }
        },
        State {
            name: "BottomEdge"
            PropertyChanges {
                target: root
                prefix: "south"
                height: root.implicitHeight
            }
            AnchorChanges {
                target: root
                anchors {
                    top: undefined
                    bottom: root.parent.bottom
                    left: root.parent.left
                    right: root.parent.right
                }
            }
            AnchorChanges {
                target: offsetHandle
                anchors {
                    top: root.top
                    bottom: undefined
                    left: undefined
                    right: undefined
                }
            }
            AnchorChanges {
                target: minimumLengthHandle
                anchors {
                    top: undefined
                    bottom: root.bottom
                    left: undefined
                    right: undefined
                }
            }
            AnchorChanges {
                target: maximumLengthHandle
                anchors {
                    top: root.top
                    bottom: undefined
                    left: undefined
                    right: undefined
                }
            }
        },
        State {
            name: "LeftEdge"
            PropertyChanges {
                target: root
                prefix: "west"
                width: root.implicitWidth
            }
            AnchorChanges {
                target: root
                anchors {
                    top: root.parent.top
                    bottom: root.parent.bottom
                    left: root.parent.left
                    right: undefined
                }
            }
            AnchorChanges {
                target: offsetHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: undefined
                    right: root.right
                }
            }
            AnchorChanges {
                target: minimumLengthHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: root.left
                    right: undefined
                }
            }
            AnchorChanges {
                target: maximumLengthHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: undefined
                    right: root.right
                }
            }
        },
        State {
            name: "RightEdge"
            PropertyChanges {
                target: root
                prefix: "east"
                width: root.implicitWidth
            }
            AnchorChanges {
                target: root
                anchors {
                    top: root.parent.top
                    bottom: root.parent.bottom
                    left: undefined
                    right: root.parent.right
                }
            }
            AnchorChanges {
                target: offsetHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: parent.left
                    right: undefined
                }
            }
            AnchorChanges {
                target: minimumLengthHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: undefined
                    right: parent.right
                }
            }
            AnchorChanges {
                target: maximumLengthHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: parent.left
                    right: undefined
                }
            }
        }
    ]
}
