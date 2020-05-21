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

Item {
    width: Layout.minimumWidth
    height: Layout.minimumHeight
    Layout.minimumWidth: units.gridUnit * 24
    Layout.minimumHeight: units.gridUnit * 21
    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight
    Layout.maximumWidth: Layout.minimumWidth
    Layout.maximumHeight: Layout.minimumHeight

    property alias activeApplet: container.activeApplet
    property alias hiddenLayout: hiddenItemsView.layout

    PlasmaExtras.PlasmoidHeading {
        id: plasmoidHeading
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: trayHeading.height + bottomPadding + container.headingHeight
    }

    PlasmaExtras.PlasmoidHeading {
        id: plasmoidFooter
        location: PlasmaExtras.PlasmoidHeading.Location.Footer
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        visible: container.appletHasFooter
        height: container.footerHeight
    }

    ColumnLayout {
        id: expandedRepresentation
        //set width/height to avoid an useless Dialog resize
        anchors.fill: parent
        spacing: plasmoidHeading.bottomPadding

        RowLayout {
            id: trayHeading

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
                        return hiddenItemsView.width + units.smallSpacing + dialog.margins.left;

                    //applet open, no sidebar
                    } else {
                        return units.smallSpacing;
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
                Layout.fillHeight: true
                Layout.topMargin: container.headingHeight
            }

            PlasmaCore.SvgItem {
                visible: hiddenItemsView.visible && activeApplet
                Layout.fillHeight: true
                Layout.preferredWidth: lineSvg.elementSize("vertical-line").width
                Layout.topMargin: container.headingHeight
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
                visible: activeApplet
                // We need to add our own margins on the top and left (when the
                // hidden items view is visible, at least) so it matches the
                //  dialog's own margins and content is centered correctly
                Layout.topMargin: mergeHeadings ? 0 : dialog.margins.top
                Layout.leftMargin: hiddenItemsView.visible && activeApplet && !LayoutMirroring.enabled ? dialog.margins.left : 0
                Layout.rightMargin: hiddenItemsView.visible && activeApplet && LayoutMirroring.enabled ? dialog.margins.right : 0
            }
        }
    }
}
