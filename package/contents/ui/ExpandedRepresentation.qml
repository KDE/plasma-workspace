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
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore

RowLayout {
    id: expandedRepresentation
    spacing: 0

    Layout.minimumWidth: Layout.minimumHeight
    Layout.minimumHeight: units.gridUnit * 22
    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight * 1.5

    property alias activeApplet: container.activeApplet
    property alias hiddenLayout: hiddenTasksColumn
    Item {
        y: units.gridUnit 
        Layout.minimumHeight: hiddenTasksColumn.implicitHeight
        Layout.minimumWidth: units.iconSizes.medium
        Layout.maximumWidth: Layout.minimumWidth
        visible: !activeApplet || activeApplet.parent.parent == hiddenTasksColumn

        CurrentItemHighLight {
            target: root.activeApplet && root.activeApplet.parent.parent == hiddenTasksColumn ? root.activeApplet.parent : null
            location: PlasmaCore.Types.LeftEdge
        }

        Column {
            id: hiddenTasksColumn
            anchors {
                left: parent.left
                right: parent.right
            }
            
            objectName: "hiddenTasksColumn"

            Repeater {
                id: hiddenTasksRepeater
                model: hiddenTasksModel

                delegate: StatusNotifierItem {}
            }
        }
    }

    PlasmaCore.SvgItem {
        visible: hiddenTasksColumn.visible || !activeApplet
        Layout.minimumWidth: lineSvg.elementSize("vertical-line").width
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        elementId: "vertical-line"

        svg: PlasmaCore.Svg {
            id: lineSvg;
            imagePath: "widgets/line"
        }
    }

    PlasmoidPopupsContainer {
        id: container
        Layout.fillWidth: true
        Layout.fillHeight: true
    }
}
