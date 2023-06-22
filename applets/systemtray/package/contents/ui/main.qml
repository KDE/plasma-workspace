/*
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Konrad Materka <materka@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Window 2.15
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.ksvg 1.0 as KSvg
import org.kde.plasma.plasmoid 2.0
import org.kde.draganddrop 2.0 as DnD
import org.kde.kirigami 2.5 as Kirigami // For Settings.tabletMode

import "items"

ContainmentItem {
    id: root

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical

    Layout.minimumWidth: vertical ? PlasmaCore.Units.iconSizes.small : mainLayout.implicitWidth + PlasmaCore.Units.smallSpacing
    Layout.minimumHeight: vertical ? mainLayout.implicitHeight + PlasmaCore.Units.smallSpacing : PlasmaCore.Units.iconSizes.small

    LayoutMirroring.enabled: !vertical && Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    readonly property alias systemTrayState: systemTrayState
    readonly property alias itemSize: tasksGrid.itemSize
    readonly property alias visibleLayout: tasksGrid
    readonly property alias hiddenLayout: expandedRepresentation.hiddenLayout
    readonly property bool oneRowOrColumn: tasksGrid.rowsOrColumns === 1

    MouseArea {
        anchors.fill: parent

        onWheel: {
            // Don't propagate unhandled wheel events
            wheel.accepted = true;
        }

        SystemTrayState {
            id: systemTrayState
        }

        //being there forces the items to fully load, and they will be reparented in the popup one by one, this item is *never* visible
        Item {
            id: preloadedStorage
            visible: false
        }

        CurrentItemHighLight {
            location: Plasmoid.location
            parent: root
        }

        DnD.DropArea {
            anchors.fill: parent

            preventStealing: true

            /** Extracts the name of the system tray applet in the drag data if present
            * otherwise returns null*/
            function systemTrayAppletName(event) {
                if (event.mimeData.formats.indexOf("text/x-plasmoidservicename") < 0) {
                    return null;
                }
                const plasmoidId = event.mimeData.getDataAsByteArray("text/x-plasmoidservicename");

                if (!Plasmoid.isSystemTrayApplet(plasmoidId)) {
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
                const plasmoidId = systemTrayAppletName(event);
                if (!plasmoidId) {
                    event.ignore();
                    return;
                }

                if (Plasmoid.configuration.extraItems.indexOf(plasmoidId) < 0) {
                    const extraItems = Plasmoid.configuration.extraItems;
                    extraItems.push(plasmoidId);
                    Plasmoid.configuration.extraItems = extraItems;
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

            Grid {
                id: tasksGrid

                Layout.maximumWidth: root.vertical ? parent.width : -1
                Layout.maximumHeight: !root.vertical ? parent.height : -1
                Layout.alignment: Qt.AlignCenter

                columns: root.vertical ? rowsOrColumns: -1
                rows: root.vertical ? -1 : rowsOrColumns
                spacing: 0
                flow: root.vertical ? Grid.LeftToRight : Grid.TopToBottom

                // The icon size to display when not using the auto-scaling setting
                readonly property int smallIconSize: PlasmaCore.Units.iconSizes.smallMedium

                // Automatically use autoSize setting when in tablet mode, if it's
                // not already being used
                readonly property bool autoSize: Plasmoid.configuration.scaleIconsToFit || Kirigami.Settings.tabletMode

                readonly property int gridThickness: root.vertical ? root.width : root.height
                // Should change to 2 rows/columns on a 56px panel (in standard DPI)
                readonly property int rowsOrColumns: autoSize ? 1 : Math.max(1, Math.min(repeater.count, Math.floor(gridThickness / (smallIconSize + PlasmaCore.Units.smallSpacing))))

                // Add margins only if the panel is larger than a small icon (to avoid large gaps between tiny icons)
                readonly property int cellSpacing: PlasmaCore.Units.smallSpacing * (Kirigami.Settings.tabletMode ? 6 : Plasmoid.configuration.iconSpacing)
                readonly property int smallSizeCellLength: gridThickness < smallIconSize ? smallIconSize : smallIconSize + cellSpacing

                readonly property int itemSize: {
                    if (autoSize) {
                        return PlasmaCore.Units.roundToIconSize(Math.min(Math.min(root.width, root.height) / rowsOrColumns, PlasmaCore.Units.iconSizes.enormous))
                    } else {
                        return smallIconSize
                    }
                }

                Repeater {
                    id: repeater

                    model: PlasmaCore.SortFilterModel {
                        sourceModel: Plasmoid.systemTrayModel
                        filterRole: "effectiveStatus"
                        filterCallback: (source_row, value) => value === PlasmaCore.Types.ActiveStatus
                    }

                    delegate: ItemLoader {
                        id: delegate

                        minLabelHeight: 0

                        // FIXME: for debugging; remove
                        Rectangle {
                            color: "red"
                            opacity: 0.5
                            anchors.fill: parent
                        }
                        width: tasksGrid.autoSize ? root.width : tasksGrid.itemSize

                        // FIXME: In vertical mode, need to fill width so there are no dead click areas,
                        // while also centering the icon in the rectangle
                        // Maybe need to make a dummy parent Item and center the ItemLoader inside it?
                        // FIXME: test and fix horizontal mode; not tested yet
                        width: {
                            if (root.vertical) {
                                return tasksGrid.autoSize ? tasksGrid.itemSize + (tasksGrid.gridThickness < tasksGrid.itemSize ? 0 : tasksGrid.cellSpacing) : tasksGrid.smallSizeCellLength
                            } else {
                                return tasksGrid.autoSize ? root.height : Math.floor(root.height / tasksGrid.rowsOrColumns)
                            }
                        }
                        // FIXME: In vertical mode, need to fill width so there are no dead click areas,
                        // while also centering the icon in the rectangle
                        // Maybe need to make a dummy parent Item and center the ItemLoader inside it?
                        // FIXME: test and fix horizontal mode; not tested yet
                        height: {
                            if (root.vertical) {
                                return tasksGrid.autoSize ? root.width : Math.floor(root.width / tasksGrid.rowsOrColumns)
                            } else {
                                return tasksGrid.autoSize ? tasksGrid.itemSize + (tasksGrid.gridThickness < tasksGrid.itemSize ? 0 : tasksGrid.cellSpacing) : tasksGrid.smallSizeCellLength
                            }
                        }

                        // We need to recalculate the stacking order of the z values due to how keyboard navigation works
                        // the tab order depends exclusively from this, so we redo it as the position in the list
                        // ensuring tab navigation focuses the expected items
                        Component.onCompleted: {
                            let item = repeater.itemAt(index - 1);
                            if (item) {
                                Plasmoid.stackItemBefore(delegate, item)
                            } else {
                                item = repeater.itemAt(index + 1);
                            }
                            if (item) {
                                Plasmoid.stackItemAfter(delegate, item)
                            }
                        }
                    }
                }

                // FIXME: This makes a bunch of items be invisible for some reason
                // add: Transition {
                //     enabled: itemSize > 0
                // 
                //     NumberAnimation {
                //         property: "scale"
                //         from: 0
                //         to: 1
                //         easing.type: Easing.InOutQuad
                //         duration: PlasmaCore.Units.longDuration
                //     }
                // }

                move: Transition {
                    NumberAnimation {
                        properties: "x,y"
                        easing.type: Easing.InOutQuad
                        duration: PlasmaCore.Units.longDuration
                    }
                }
            }

            ExpanderArrow {
                id: expander
                Layout.fillWidth: vertical
                Layout.fillHeight: !vertical
                iconSize: tasksGrid.itemSize
                visible: root.hiddenLayout.itemCount > 0
            }
        }

        Timer {
            id: expandedSync
            interval: 100
            onTriggered: systemTrayState.expanded = dialog.visible;
        }

        //Main popup
        PlasmaCore.Dialog {
            id: dialog
            objectName: "popupWindow"
            visualParent: root
            flags: Qt.WindowStaysOnTopHint
            location: Plasmoid.location
            hideOnWindowDeactivate: !Plasmoid.configuration.pin
            visible: systemTrayState.expanded
            // visualParent: implicitly set to parent
            backgroundHints: (Plasmoid.containmentDisplayHints & PlasmaCore.Types.DesktopFullyCovered) ? PlasmaCore.Dialog.SolidBackground : PlasmaCore.Dialog.StandardBackground
            type: PlasmaCore.Dialog.AppletPopup
            appletInterface: root

            onVisibleChanged: {
                if (!visible) {
                    expandedSync.restart();
                } else {
                    if (expandedRepresentation.plasmoidContainer.visible) {
                        expandedRepresentation.plasmoidContainer.forceActiveFocus();
                    } else if (expandedRepresentation.hiddenLayout.visible) {
                        expandedRepresentation.hiddenLayout.forceActiveFocus();
                    }
                }
            }
            mainItem: ExpandedRepresentation {
                id: expandedRepresentation

                Keys.onEscapePressed: {
                    systemTrayState.expanded = false
                }

                // Draws a line between the applet dialog and the panel
                KSvg.SvgItem {
                    // Only draw for popups of panel applets, not desktop applets
                    visible: [PlasmaCore.Types.TopEdge, PlasmaCore.Types.LeftEdge, PlasmaCore.Types.RightEdge, PlasmaCore.Types.BottomEdge]
                        .includes(Plasmoid.location)
                    anchors {
                        top: Plasmoid.location === PlasmaCore.Types.BottomEdge ? undefined : parent.top
                        left: Plasmoid.location === PlasmaCore.Types.RightEdge ? undefined : parent.left
                        right: Plasmoid.location === PlasmaCore.Types.LeftEdge ? undefined : parent.right
                        bottom: Plasmoid.location === PlasmaCore.Types.TopEdge ? undefined : parent.bottom
                        topMargin: Plasmoid.location === PlasmaCore.Types.BottomEdge ? undefined : -dialog.margins.top
                        leftMargin: Plasmoid.location === PlasmaCore.Types.RightEdge ? undefined : -dialog.margins.left
                        rightMargin: Plasmoid.location === PlasmaCore.Types.LeftEdge ? undefined : -dialog.margins.right
                        bottomMargin: Plasmoid.location === PlasmaCore.Types.TopEdge ? undefined : -dialog.margins.bottom
                    }
                    height: (Plasmoid.location === PlasmaCore.Types.TopEdge || Plasmoid.location === PlasmaCore.Types.BottomEdge) ? PlasmaCore.Units.devicePixelRatio : undefined
                    width: (Plasmoid.location === PlasmaCore.Types.LeftEdge || Plasmoid.location === PlasmaCore.Types.RightEdge) ? PlasmaCore.Units.devicePixelRatio : undefined
                    z: 999 /* Draw the line on top of the applet */
                    elementId: (Plasmoid.location === PlasmaCore.Types.TopEdge || Plasmoid.location === PlasmaCore.Types.BottomEdge) ? "horizontal-line" : "vertical-line"
                    svg: KSvg.Svg {
                        imagePath: "widgets/line"
                    }
                }

                LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                LayoutMirroring.childrenInherit: true
            }
        }
    }
}
