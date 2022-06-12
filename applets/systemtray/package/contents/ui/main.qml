/*
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Konrad Materka <materka@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Window 2.15
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.draganddrop 2.0 as DnD
import org.kde.kirigami 2.5 as Kirigami // For Settings.tabletMode

import "items"

MouseArea {
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

            if (!Plasmoid.nativeInterface.isSystemTrayApplet(plasmoidId)) {
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

        GridView {
            id: tasksGrid

            Layout.alignment: Qt.AlignCenter

            interactive: false //disable features we don't need
            flow: vertical ? GridView.LeftToRight : GridView.TopToBottom

            // The icon size to display when not using the auto-scaling setting
            readonly property int smallIconSize: PlasmaCore.Units.iconSizes.smallMedium

            // Automatically use autoSize setting when in tablet mode, if it's
            // not already being used
            readonly property bool autoSize: Plasmoid.configuration.scaleIconsToFit || Kirigami.Settings.tabletMode

            readonly property int gridThickness: root.vertical ? root.width : root.height
            // Should change to 2 rows/columns on a 56px panel (in standard DPI)
            readonly property int rowsOrColumns: autoSize ? 1 : Math.max(1, Math.min(count, Math.floor(gridThickness / (smallIconSize + PlasmaCore.Units.smallSpacing))))

            // Add margins only if the panel is larger than a small icon (to avoid large gaps between tiny icons)
            readonly property int cellSpacing: PlasmaCore.Units.smallSpacing * (Kirigami.Settings.tabletMode ? 6 : Plasmoid.configuration.iconSpacing)
            readonly property int smallSizeCellLength: gridThickness < smallIconSize ? smallIconSize : smallIconSize + cellSpacing

            cellHeight: {
                if (root.vertical) {
                    return autoSize ? itemSize + (gridThickness < itemSize ? 0 : cellSpacing) : smallSizeCellLength
                } else {
                    return autoSize ? root.height : Math.floor(root.height / rowsOrColumns)
                }
            }
            cellWidth: {
                if (root.vertical) {
                    return autoSize ? root.width : Math.floor(root.width / rowsOrColumns)
                } else {
                    return autoSize ? itemSize + (gridThickness < itemSize ? 0 : cellSpacing) : smallSizeCellLength
                }
            }

            //depending on the form factor, we are calculating only one dimension, second is always the same as root/parent
            implicitHeight: root.vertical ? cellHeight * Math.ceil(count / rowsOrColumns) : root.height
            implicitWidth: !root.vertical ? cellWidth * Math.ceil(count / rowsOrColumns) : root.width

            readonly property int itemSize: {
                if (autoSize) {
                    return PlasmaCore.Units.roundToIconSize(Math.min(Math.min(root.width, root.height) / rowsOrColumns, PlasmaCore.Units.iconSizes.enormous))
                } else {
                    return smallIconSize
                }
            }

            model: PlasmaCore.SortFilterModel {
                sourceModel: Plasmoid.nativeInterface.systemTrayModel
                filterRole: "effectiveStatus"
                filterCallback: (source_row, value) => value === PlasmaCore.Types.ActiveStatus
            }

            delegate: ItemLoader {
                id: delegate
                // We need to recalculate the stacking order of the z values due to how keyboard navigation works
                // the tab order depends exclusively from this, so we redo it as the position in the list
                // ensuring tab navigation focuses the expected items
                Component.onCompleted: {
                    let item = tasksGrid.itemAtIndex(index - 1);
                    if (item) {
                        Plasmoid.nativeInterface.stackItemBefore(delegate, item)
                    } else {
                        item = tasksGrid.itemAtIndex(index + 1);
                    }
                    if (item) {
                        Plasmoid.nativeInterface.stackItemAfter(delegate, item)
                    }
                }
            }

            add: Transition {
                enabled: itemSize > 0

                NumberAnimation {
                    property: "scale"
                    from: 0
                    to: 1
                    easing.type: Easing.InOutQuad
                    duration: PlasmaCore.Units.longDuration
                }
            }

            displaced: Transition {
                //ensure scale value returns to 1.0
                //https://doc.qt.io/qt-5/qml-qtquick-viewtransition.html#handling-interrupted-animations
                NumberAnimation {
                    property: "scale"
                    to: 1
                    easing.type: Easing.InOutQuad
                    duration: PlasmaCore.Units.longDuration
                }
            }

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
            Layout.alignment: vertical ? Qt.AlignVCenter : Qt.AlignHCenter
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
        appletInterface: Plasmoid.self

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
            PlasmaCore.SvgItem {
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
                svg: PlasmaCore.Svg {
                    imagePath: "widgets/line"
                }
            }

            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
        }
    }
}
