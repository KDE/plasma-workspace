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
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: expandedRepresentation

    Layout.minimumWidth: Layout.minimumHeight
    Layout.minimumHeight: units.gridUnit * 20
    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight * 1.5

    property alias activeApplet: container.activeApplet
    property alias hiddenLayout: hiddenTasksColumn


    PlasmaExtras.Heading {
        id: heading
        level: 1

        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
            leftMargin: hiddenTasksView.visible ? hiddenTasksColumn.width + units.smallSpacing : 0
        }

        text: activeApplet ? activeApplet.title : i18n("Status & Notifications")
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (activeApplet) {
                    activeApplet.expanded = false;
                    dialog.visible = true;
                }
            }
        }
    }

    PlasmaCore.SvgItem {
        visible: hiddenTasksColumn.visible && activeApplet
        width: lineSvg.elementSize("vertical-line").width
        x: hiddenTasksColumn.width
        anchors {
            top: parent.top
            bottom: parent.bottom
            margins: -units.gridUnit
        }

        elementId: "vertical-line"

        svg: PlasmaCore.Svg {
            id: lineSvg;
            imagePath: "widgets/line"
        }
    }

    PlasmaExtras.ScrollArea {
        id: hiddenTasksView
        anchors {
            left: parent.left
            top: heading.bottom
            bottom: parent.bottom
        }
        visible: !activeApplet || activeApplet.parent.parent == hiddenTasksColumn
        width: activeApplet ? units.iconSizes.smallMedium : parent.width

        Flickable {
            contentWidth: width
            contentHeight: hiddenTasksColumn.height

            Item {
                width: hiddenTasksColumn.width
                height: hiddenTasksColumn.height

                CurrentItemHighLight {
                    target: root.activeApplet && root.activeApplet.parent.parent == hiddenTasksColumn ? root.activeApplet.parent : null
                    location: PlasmaCore.Types.LeftEdge
                }

                Column {
                    id: hiddenTasksColumn
                    spacing: units.smallSpacing
                    width: units.iconSizes.smallMedium
                    
                    objectName: "hiddenTasksColumn"

                    Repeater {
                        id: hiddenTasksRepeater
                        model: hiddenTasksModel

                        delegate: StatusNotifierItem {}
                    }
                }
            }
        }
    }

    PlasmoidPopupsContainer {
        id: container
        anchors {
            left: parent.left
            right: parent.right
            top: heading.bottom
            bottom: parent.bottom
            leftMargin: hiddenTasksView.visible ? units.iconSizes.smallMedium + units.smallSpacing : 0
        }
    }
}
