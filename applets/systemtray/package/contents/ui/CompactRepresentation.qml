/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddonsComponents

import org.kde.private.systemtray 2.0 as SystemTray


Item {
    id: compactRepresentation

    Layout.minimumWidth: !root.vertical ? computeDimension() : 0
    Layout.minimumHeight: !root.vertical ? 0 : computeDimensionHeight()
    Layout.maximumWidth: Layout.minimumWidth
    Layout.maximumHeight: Layout.minimumHeight
    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight

    Layout.fillWidth: false
    Layout.fillHeight: false

    property QtObject systrayhost: undefined

    Connections {
        target: root
        onExpandedTaskChanged: {
            parent.expandedFeedback.opacity = Qt.binding(function() {
                return root.expandedTask == null && plasmoid.expanded ? 1 : 0
            })
        }
    }

    function computeDimension() {
        var dim = root.vertical ? compactRepresentation.width : compactRepresentation.height
        var rows = Math.floor(dim / root.itemSize);
        var cols = Math.ceil(gridView.count / rows);
        var res = cols * (root.itemSize + units.smallSpacing*2) + units.smallSpacing + tooltip.width;
        return res;
    }

    function computeDimensionHeight() {
        var dim = root.vertical ? compactRepresentation.width : compactRepresentation.height
        var cols = Math.floor(dim / root.itemSize);
        var rows = Math.ceil(gridView.count / cols);
        var res = rows * (root.itemSize + units.smallSpacing*2) + units.smallSpacing + tooltip.height;
        return res;
    }

    function togglePopup() {
        //print("toggle popup => " + !plasmoid.expanded);
        if (!plasmoid.expanded) {
            plasmoid.expanded = true
        } else {
            if (root.expandedTask == null) {
                plasmoid.expanded = false
            }
        }
    }

    function setItemPreferredSize() {
        var dim = (root.vertical ? compactRepresentation.width : compactRepresentation.height) - units.smallSpacing;
        if (root.preferredItemSize != dim) {
            root.preferredItemSize = dim;
        }
    }

    Rectangle {
        anchors.fill: parent;
        border.width: 1;
        border.color: "black";
        color: "green";
        visible: root.debug;
        opacity: 0.2;
    }

    Component {
        id: taskDelegateComponent
        TaskDelegate {
            id: taskDelegate
            width: gridView.cellWidth
            height: gridView.cellHeight
        }
    }

    GridView {
        id: gridView
        objectName: "gridView"
        flow: root.vertical ? GridView.LeftToRight : GridView.TopToBottom

        anchors {
            top: parent.top
            bottom: !root.vertical ? parent.bottom : tooltip.top
            left: parent.left
            right: !root.vertical ? tooltip.left : parent.right
        }
        cellWidth: root.vertical ? gridView.width / Math.floor(gridView.width / root.itemSize) : root.itemSize + units.smallSpacing * 2
        cellHeight: !root.vertical ? gridView.height / Math.floor(gridView.height / root.itemSize) : root.itemSize + units.smallSpacing * 2

        interactive: false

        model: systrayhost.shownTasks

        delegate: taskDelegateComponent
    }

    // Tooltip for arrow --------------------------------
    PlasmaCore.ToolTipArea {
        id: tooltip

        width: root.vertical ? compactRepresentation.width : units.iconSizes.smallMedium
        height: !root.vertical ? compactRepresentation.height : units.iconSizes.smallMedium
        anchors {
            right: parent.right
            bottom: parent.bottom
        }

        subText: plasmoid.expanded ? i18n("Hide icons") : i18n("Show hidden icons")

        MouseArea {
            id: arrowMouseArea
            anchors.fill: parent
            onClicked: plasmoid.expanded = !plasmoid.expanded

            readonly property int arrowAnimationDuration: units.shortDuration * 3

            PlasmaCore.Svg {
                id: arrowSvg
                imagePath: "widgets/arrows"
            }

            PlasmaCore.SvgItem {
                id: arrow

                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: width

                rotation: plasmoid.expanded ? 180 : 0
                Behavior on rotation {
                    RotationAnimation {
                        duration: arrowMouseArea.arrowAnimationDuration
                    }
                }
                opacity: plasmoid.expanded ? 0 : 1
                Behavior on opacity {
                    NumberAnimation {
                        duration: arrowMouseArea.arrowAnimationDuration
                    }
                }

                svg: arrowSvg
                elementId: {
                    if (plasmoid.location == PlasmaCore.Types.BottomEdge) {
                        return "up-arrow"
                    } else if (plasmoid.location == PlasmaCore.Types.TopEdge) {
                        return "down-arrow"
                    } else if (plasmoid.location == PlasmaCore.Types.LeftEdge) {
                        return "right-arrow"
                    } else {
                        return "left-arrow"
                    }
                }
            }

            PlasmaCore.SvgItem {
                anchors.centerIn: parent
                width: arrow.width
                height: arrow.height

                rotation: plasmoid.expanded ? 0 : -180
                Behavior on rotation {
                    RotationAnimation {
                        duration: arrowMouseArea.arrowAnimationDuration
                    }
                }
                opacity: plasmoid.expanded ? 1 : 0
                Behavior on opacity {
                    NumberAnimation {
                        duration: arrowMouseArea.arrowAnimationDuration
                    }
                }

                svg: arrowSvg
                elementId: {
                    if (plasmoid.location == PlasmaCore.Types.BottomEdge) {
                        return "down-arrow"
                    } else if (plasmoid.location == PlasmaCore.Types.TopEdge) {
                        return "up-arrow"
                    } else if (plasmoid.location == PlasmaCore.Types.LeftEdge) {
                        return "left-arrow"
                    } else {
                        return "right-arrow"
                    }
                }
            }
        }
    }

    onHeightChanged: setItemPreferredSize();
    onWidthChanged: setItemPreferredSize();
}
