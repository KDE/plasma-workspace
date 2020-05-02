/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
 *   Copyright 2020 Konrad Materka <materka@gmail.com>
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

import QtQuick 2.5
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.draganddrop 2.0 as DnD
import org.kde.kirigami 2.5 as Kirigami

import "items"

MouseArea {
    id: root

    Layout.minimumWidth: vertical ? units.iconSizes.small : tasksGrid.implicitWidth + (expander.visible ? expander.implicitWidth : 0) + units.smallSpacing

    Layout.minimumHeight: vertical ? tasksGrid.implicitHeight + (expander.visible ? expander.implicitHeight : 0) + units.smallSpacing : units.smallSpacing

    Layout.preferredHeight: Layout.minimumHeight
    LayoutMirroring.enabled: !vertical && Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property var iconSizes: ["small", "smallMedium", "medium", "large", "huge", "enormous"];
    property int iconSize: plasmoid.configuration.iconSize + (Kirigami.Settings.tabletMode ? 1 : 0)

    property bool vertical: plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property int itemSize: {
        var baseSize = units.roundToIconSize(Math.min(Math.min(width, height), units.iconSizes[iconSizes[Math.min(iconSizes.length-1, iconSize)]]));
        if (Kirigami.Settings.tabletMode) {
            // Set the tray items' clickable areas on the panel to be bigger than normal to allow for easier touchability
            return baseSize + units.smallSpacing;
        } else {
            return baseSize + Math.round(units.smallSpacing/2);
        }
    }
    property int hiddenItemSize: units.iconSizes.smallMedium
    property alias expanded: dialog.visible
    property Item activeApplet
    property int status: dialog.visible ? PlasmaCore.Types.RequiresAttentionStatus : PlasmaCore.Types.PassiveStatus

    property alias visibleLayout: tasksGrid
    property alias hiddenLayout: expandedRepresentation.hiddenLayout

    Plasmoid.onExpandedChanged: {
        if (!plasmoid.expanded) {
            dialog.visible = plasmoid.expanded;
        }
    }

    onWheel: {
        // Don't propagate unhandled wheel events
        wheel.accepted = true;
    }

    //being there forces the items to fully load, and they will be reparented in the popup one by one, this item is *never* visible
    Item {
        id: preloadedStorage
        visible: false
    }

    Connections {
        target: plasmoid
        function onUserConfiguringChanged() {
            if (plasmoid.userConfiguring) {
                dialog.visible = false
            }
        }
    }

    Connections {
        target: plasmoid.configuration

        function onExtraItemsChanged() {
            plasmoid.nativeInterface.allowedPlasmoids = plasmoid.configuration.extraItems
        }
    }

    CurrentItemHighLight {
        readonly property bool visibleAppletActivated: root.activeApplet && root.activeApplet.parent && root.activeApplet.parent.inVisibleLayout
        parent: visibleAppletActivated ? root.activeApplet.parent : root
        target: visibleAppletActivated ? root.activeApplet.parent : root
        location: plasmoid.location
    }

    DnD.DropArea {
        anchors.fill: parent

        preventStealing: true;

        /** Extracts the name of the system tray applet in the drag data if present
         * otherwise returns null*/
        function systemTrayAppletName(event) {
            if (event.mimeData.formats.indexOf("text/x-plasmoidservicename") < 0) {
                return null;
            }
            var plasmoidId = event.mimeData.getDataAsByteArray("text/x-plasmoidservicename");

            if (!plasmoid.nativeInterface.isSystemTrayApplet(plasmoidId)) {
                return null;
            }
            return plasmoidId;
        }

        onDragEnter: {
            if (!systemTrayAppletName(event)) {
                event.ignore();
            }
        }

        onDrop: {
            var plasmoidId = systemTrayAppletName(event);
            if (!plasmoidId) {
                event.ignore();
                return;
            }

            if (plasmoid.configuration.extraItems.indexOf(plasmoidId) < 0) {
                var extraItems = plasmoid.configuration.extraItems;
                extraItems.push(plasmoidId);
                plasmoid.configuration.extraItems = extraItems;
            }
        }
    }

    //Main Layout
    GridLayout {
        id: mainLayout

        rowSpacing: 0
        columnSpacing: 0
        anchors.fill: parent

        flow: vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight

        GridView {
            id: tasksGrid
            Layout.alignment: Qt.AlignCenter

            interactive: false //disable features we don't need
            flow: vertical ? GridView.LeftToRight : GridView.TopToBottom

            cellHeight: Math.min(root.itemSize + units.smallSpacing, root.height)
            cellWidth: Math.min(root.itemSize + units.smallSpacing, root.width)

            readonly property int columns: !vertical ? Math.ceil(count / rows)
                                           : Math.max(1, Math.floor(root.width / cellWidth))
            readonly property int rows: vertical ? Math.ceil(count / columns)
                                           : Math.max(1, Math.floor(root.height / cellHeight))

            implicitHeight: rows * cellHeight
            implicitWidth: columns * cellWidth

            model: PlasmaCore.SortFilterModel {
                sourceModel: plasmoid.nativeInterface.systemTrayModel
                filterRole: "effectiveStatus"
                filterCallback: function(source_row, value) {
                    return value === PlasmaCore.Types.ActiveStatus
                }
            }

            delegate: ItemLoader {}

            add: Transition {
                enabled: root.itemSize > 0

                NumberAnimation {
                    property: "scale"
                    from: 0
                    to: 1
                    easing.type: Easing.InOutQuad
                    duration: units.longDuration
                }
            }

            displaced: Transition {
                //ensure scale value returns to 1.0
                //https://doc.qt.io/qt-5/qml-qtquick-viewtransition.html#handling-interrupted-animations
                NumberAnimation {
                    property: "scale"
                    to: 1
                    easing.type: Easing.InOutQuad
                    duration: units.longDuration
                }
            }

            move: Transition {
                NumberAnimation {
                    properties: "x,y"
                    easing.type: Easing.InOutQuad
                    duration: units.longDuration
                }
            }
        }

        ExpanderArrow {
            id: expander
            Layout.fillWidth: vertical
            Layout.fillHeight: !vertical
        }
    }

    //Main popup
    PlasmaCore.Dialog {
        id: dialog
        visualParent: root
        flags: Qt.WindowStaysOnTopHint
        location: plasmoid.location
        hideOnWindowDeactivate: !plasmoid.configuration.pin

        onVisibleChanged: {
            if (!visible) {
                plasmoid.status = PlasmaCore.Types.PassiveStatus;
                if (root.activeApplet) {
                    root.activeApplet.expanded = false;
                }
            } else {
                plasmoid.status = PlasmaCore.Types.RequiresAttentionStatus;
            }
            plasmoid.expanded = visible;
        }
        mainItem: ExpandedRepresentation {
            id: expandedRepresentation

            Keys.onEscapePressed: {
                root.expanded = false;
            }

            activeApplet: root.activeApplet

            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
        }
    }
}
