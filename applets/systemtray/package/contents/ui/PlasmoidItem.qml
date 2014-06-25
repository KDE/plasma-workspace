/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.0
import org.kde.private.systemtray 2.0 as SystemTray

Item {
    id: plasmoidItem
    // Notify the plasmoids inside containers of location changes
    Connections {
        target: plasmoid
        onLocationChanged: {
            if (modelData.taskType == SystemTray.Task.TypePlasmoid) {
                setLocation(plasmoid.location);
            }
        }
    }

    Connections {
        target: modelData
        onExpandedChanged: {
            if (modelData.expanded) {
                plasmoid.expanded = true;
            }
        }
    }
    MouseArea {
        anchors {
            fill: parent
        }
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            taskItemContainer.hideToolTip();
            if (mouse.button == Qt.LeftButton) {
                if (modelData.expanded) {
                    if (plasmoidItem.parent.parent.parent.objectName == "taskListDelegate") {
                        modelData.expanded = false;
                    } else {
                        plasmoid.expanded = false;
                    }
                } else {
                    modelData.expanded = true;
                }

            } else if (mouse.button == Qt.RightButton) {
                modelData.showMenu(mouse.x, mouse.y);
            }
        }
    }
}
