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

import QtQuick 2.12
import QtQuick.Layouts 1.12

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

ColumnLayout {
    id: expandedRepresentation
    //set width/height to avoid an useless Dialog resize
    width: Layout.minimumWidth
    height: Layout.minimumHeight
    Layout.minimumWidth: units.gridUnit * 24
    Layout.minimumHeight: units.gridUnit * 21
    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight
    Layout.maximumWidth: Layout.minimumWidth
    Layout.maximumHeight: Layout.minimumHeight
    spacing: 0 // avoid gap between title and content

    property alias activeApplet: container.activeApplet
    property alias hiddenLayout: hiddenItemsView.layout

    RowLayout {

        PlasmaExtras.Heading {
            id: heading
            Layout.fillWidth: true
            level: 1
            Layout.leftMargin: {
                //Menu mode
                if (!activeApplet && hiddenItemsView.visible && !LayoutMirroring.enabled) {
                    return units.smallSpacing;

                //applet open, sidebar
                } else if (activeApplet && hiddenItemsView.visible && !LayoutMirroring.enabled) {
                    return hiddenItemsView.width + units.largeSpacing;

                //applet open, no sidebar
                } else {
                    return 0;
                }
            }
            Layout.rightMargin: {
                //Menu mode
                if (!activeApplet && hiddenItemsView.visible && LayoutMirroring.enabled) {
                    return units.smallSpacing;

                //applet open, sidebar
                } else if (activeApplet && hiddenItemsView.visible && LayoutMirroring.enabled) {
                    return hiddenItemsView.width + units.largeSpacing;

                //applet open, no sidebar
                } else {
                    return 0;
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
            visible: !activeApplet
            Layout.fillWidth: true
            level: 1
            text: i18n("Status and Notifications")
        }

        PlasmaComponents.ToolButton {
            id: pinButton
            Layout.preferredHeight: Math.round(units.gridUnit * 1.25)
            Layout.preferredWidth: Layout.preferredHeight
            checkable: true
            checked: plasmoid.configuration.pin
            onToggled: plasmoid.configuration.pin = checked
            icon.name: "window-pin"
            PlasmaComponents.ToolTip {
                text: i18n("Keep Open")
            }
        }
    }

    RowLayout {
        spacing: 0 // must be 0 so that the separator is as close to the indicator as possible

        HiddenItemsView {
            id: hiddenItemsView
            Layout.fillWidth: !activeApplet
            Layout.preferredWidth: activeApplet ? iconColumnWidth : -1
            Layout.fillHeight: true
        }

        PlasmaCore.SvgItem {
            visible: hiddenItemsView.visible && activeApplet
            Layout.fillHeight: true
            Layout.preferredWidth: lineSvg.elementSize("vertical-line").width
            elementId: "vertical-line"
            svg: PlasmaCore.Svg {
                id: lineSvg;
                imagePath: "widgets/line"
            }
        }

        PlasmoidPopupsContainer {
            id: container
            visible: activeApplet
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: hiddenItemsView.visible && activeApplet && !LayoutMirroring.enabled ? units.largeSpacing : 0
            Layout.rightMargin: hiddenItemsView.visible && activeApplet && LayoutMirroring.enabled ? units.largeSpacing : 0
        }
    }
}
