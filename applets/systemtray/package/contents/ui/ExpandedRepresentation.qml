/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
 *   Copyright 2020 Nate Graham <nate@kde.org>
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
    id: popup
    //set width/height to avoid useless Dialog resize
    readonly property int defaultWidth: units.gridUnit * 24
    readonly property int defaultHeight: units.gridUnit * 24

    width: defaultWidth
    Layout.minimumWidth: defaultWidth
    Layout.preferredWidth: defaultWidth
    Layout.maximumWidth: defaultWidth

    height: defaultHeight
    Layout.minimumHeight: defaultHeight
    Layout.preferredHeight: defaultHeight
    Layout.maximumHeight: defaultHeight

    property alias activeApplet: container.activeApplet
    property alias hiddenLayout: hiddenItemsView.layout

    // Header
    PlasmaExtras.PlasmoidHeading {
        id: plasmoidHeading
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: trayHeading.height + bottomPadding + container.headingHeight
    }

    // Main content layout
    ColumnLayout {
        id: expandedRepresentation
        anchors.fill: parent
        // TODO: remove this so the scrollview fully touches the header;
        // add top padding internally
        spacing: plasmoidHeading.bottomPadding

        // Header content layout
        RowLayout {
            id: trayHeading

            PlasmaComponents.ToolButton {
                id: backButton
                visible: activeApplet && activeApplet.expanded && (hiddenItemsView.itemCount > 0)
                icon.name: "go-previous"
                onClicked: {
                    if (activeApplet) {
                        activeApplet.expanded = false;
                        dialog.visible = true;
                    }
                }
            }

            PlasmaExtras.Heading {
                Layout.fillWidth: true

                level: 1
                text: activeApplet ? activeApplet.title : i18n("Status and Notifications")
            }

            PlasmaComponents.ToolButton {
                // Don't show when displaying an applet's own view since then
                // there would be two configure buttons and that would be weird
                // TODO: in the future make this button context-sensitive so
                // that it triggers the config action for whatever applet is
                // being viewed, and then hide the applet's own config button
                // if both would be shown at the same time
                visible: !activeApplet && plasmoid.action("configure").enabled
                icon.name: "configure"
                onClicked: plasmoid.action("configure").trigger()
                PlasmaComponents.ToolTip {
                    text: plasmoid.action("configure").text
                }
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

        // Grid view of all available items
        HiddenItemsView {
            id: hiddenItemsView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: units.smallSpacing
            visible: !activeApplet
        }

        // Container for currently visible item
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

    // Footer
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
        // So that it doesn't appear over the content view, which results in
        // the footer controls being inaccessible
        z: -9999
    }
}
