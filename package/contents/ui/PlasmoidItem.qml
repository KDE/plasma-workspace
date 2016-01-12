/*
 *   Copyright 2015 Marco Martin <mart@kde.org>
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

Item {
    id: plasmoidContainer
    width: root.itemWidth
    height: root.itemHeight
    property Item applet

    Connections {
        target: applet
        onExpandedChanged: {
            if (expanded) {
                root.activeApplet = applet;
                dialog.visible = true;
            }
        }
        onStatusChanged: {
            if (applet.status == PlasmaCore.Types.PassiveStatus) {
                plasmoidContainer.parent = hiddenLayout;
            } else {
                plasmoidContainer.parent = visibleLayout;
            }
        }
    }
    PlasmaComponents.Label {
        visible: applet.parent.parent.objectName == "hiddenTasksColumn" && !root.activeApplet
        anchors {
            left: parent.right
            verticalCenter: parent.verticalCenter
        }
        text: applet.title
    }
}
