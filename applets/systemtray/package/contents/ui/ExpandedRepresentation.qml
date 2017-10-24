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
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: expandedRepresentation

    Layout.minimumWidth: units.gridUnit * 24
    Layout.minimumHeight: units.gridUnit * 21
    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight * 1.5

    property alias activeApplet: container.activeApplet
    property alias hiddenLayout: hiddenItemsView.layout

    PlasmaComponents.ToolButton {
        id: pinButton
        anchors.right: parent.right
        width: Math.round(units.gridUnit * 1.25)
        height: width
        checkable: true
        checked: plasmoid.configuration.pin
        onCheckedChanged: plasmoid.configuration.pin = checked
        iconSource: "window-pin"
        z: 2
    }

    PlasmaExtras.Heading {
        id: heading
        level: 1

        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
            topMargin: hiddenItemsView.visible ? units.smallSpacing : 0
            leftMargin: {
                //Menu mode
                if (!activeApplet && hiddenItemsView.visible) {
                    return units.smallSpacing;

                //applet open, sidebar
                } else if (activeApplet && hiddenItemsView.visible) {
                    return hiddenItemsView.iconColumnWidth + units.largeSpacing;

                //applet open, no sidebar
                } else {
                    return 0;
                }
            }
        }

        visible: activeApplet
        text: activeApplet ? activeApplet.title : ""
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
    PlasmaExtras.Heading {
        id: noAppletHeading
        level: 1
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
            topMargin: hiddenItemsView.visible ? units.smallSpacing : 0
            leftMargin: hiddenItemsView.iconColumnWidth + units.largeSpacing;
        }
        text: i18n("Status & Notifications")
        visible: !heading.visible
    }

    PlasmaCore.SvgItem {
        anchors {
            left: parent.left
            leftMargin: hiddenLayout.width
            top: parent.top
            bottom: parent.bottom
            margins: -units.gridUnit
        }

        visible: hiddenItemsView.visible && activeApplet
        width: lineSvg.elementSize("vertical-line").width

        elementId: "vertical-line"

        svg: PlasmaCore.Svg {
            id: lineSvg;
            imagePath: "widgets/line"
        }
    }

    HiddenItemsView {
        id: hiddenItemsView
        anchors {
            left: parent.left
            top: noAppletHeading.bottom
            topMargin: units.smallSpacing
            bottom: parent.bottom
        }
    }

    PlasmoidPopupsContainer {
        id: container
        anchors {
            left: parent.left
            right: parent.right
            top: heading.bottom
            bottom: parent.bottom
            leftMargin: hiddenItemsView.visible ? hiddenItemsView.iconColumnWidth + units.largeSpacing : 0
        }
    }
}
