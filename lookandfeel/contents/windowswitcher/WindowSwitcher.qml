/*
 KWin - the KDE window manager
 This file is part of the KDE project.

 SPDX-FileCopyrightText: 2020 Chris Holland <zrenfire@gmail.com>

 SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kwin 2.0 as KWin

// https://techbase.kde.org/Development/Tutorials/KWin/WindowSwitcher
// https://github.com/KDE/kwin/blob/master/tabbox/switcheritem.h
KWin.Switcher {
    id: tabBox
    currentIndex: thumbnailGridView.currentIndex

    PlasmaCore.Dialog {
        id: dialog
        location: PlasmaCore.Types.Floating
        visible: tabBox.visible
        flags: Qt.X11BypassWindowManagerHint
        x: tabBox.screenGeometry.x + tabBox.screenGeometry.width * 0.5 - dialogMainItem.width * 0.5
        y: tabBox.screenGeometry.y + tabBox.screenGeometry.height * 0.5 - dialogMainItem.height * 0.5

        onVisibleChanged: {
            if (visible) {
                dialogMainItem.calculateColumnCount();
            } else {
                thumbnailGridView.highCount = 0;
            }
        }

        mainItem: Item {
            id: dialogMainItem

            focus: true

            property int maxWidth: tabBox.screenGeometry.width * 0.9
            property int maxHeight: tabBox.screenGeometry.height * 0.7
            property real screenFactor: tabBox.screenGeometry.width / tabBox.screenGeometry.height
            property int maxGridColumnsByWidth: Math.floor(maxWidth / thumbnailGridView.cellWidth)

            property int gridColumns: maxGridColumnsByWidth
            property int gridRows: Math.ceil(thumbnailGridView.count / gridColumns)
            property int optimalWidth: thumbnailGridView.cellWidth * gridColumns
            property int optimalHeight: thumbnailGridView.cellHeight * gridRows
            property bool canStretchX: false
            property bool canStretchY: false
            width: Math.min(Math.max(thumbnailGridView.cellWidth, optimalWidth), maxWidth)
            height: Math.min(Math.max(thumbnailGridView.cellHeight, optimalHeight), maxHeight)

            clip: true

            // Simple greedy algorithm
            function calculateColumnCount() {
                // respect screenGeometry
                const c = Math.min(thumbnailGridView.count, maxGridColumnsByWidth);

                const residue = thumbnailGridView.count % c;
                if (residue == 0) {
                    gridColumns = c;
                    return;
                }

                // start greedy recursion
                gridColumns = columnCountRecursion(c, c, c - residue);
            }

            // Step for greedy algorithm
            function columnCountRecursion(prevC, prevBestC, prevDiff) {
                const c = prevC - 1;

                // don't increase vertical extent more than horizontal
                // and don't exceed maxHeight
                if (prevC * prevC <= thumbnailGridView.count + prevDiff ||
                        maxHeight < Math.ceil(thumbnailGridView.count / c) * thumbnailGridView.cellHeight) {
                    return prevBestC;
                }
                const residue = thumbnailGridView.count % c;
                // halts algorithm at some point
                if (residue == 0) {
                    return c;
                }
                // empty slots
                const diff = c - residue;

                // compare it to previous count of empty slots
                if (diff < prevDiff) {
                    return columnCountRecursion(c, c, diff);
                } else if (diff == prevDiff) {
                    // when it's the same try again, we'll stop early enough thanks to the landscape mode condition
                    return columnCountRecursion(c, prevBestC, diff);
                }
                // when we've found a local minimum choose this one (greedy)
                return columnCountRecursion(c, prevBestC, diff);
            }

            // Just to get the margin sizes
            PlasmaCore.FrameSvgItem {
                id: hoverItem
                imagePath: "widgets/viewitem"
                prefix: "hover"
                visible: false
            }

            GridView {
                id: thumbnailGridView
                anchors.fill: parent

                model: tabBox.model

                property int iconSize: units.iconSizes.smallMedium
                property int captionRowHeight: 30 * units.devicePixelRatio // The close button is 30x30 in Breeze
                property int thumbnailWidth: 300 * units.devicePixelRatio
                property int thumbnailHeight: thumbnailWidth * (1.0/dialogMainItem.screenFactor)
                cellWidth: hoverItem.margins.left + thumbnailWidth + hoverItem.margins.right
                cellHeight: hoverItem.margins.top + captionRowHeight + thumbnailHeight + hoverItem.margins.bottom

                keyNavigationWraps: true
                highlightMoveDuration: 0

                // Allow expansion on increasing count
                property int highCount: 0
                onCountChanged: {
                    if (highCount < count) {
                        dialogMainItem.calculateColumnCount();
                        highCount = count;
                    }
                }

                delegate: Item {
                    id: thumbnailGridItem
                    width: thumbnailGridView.cellWidth
                    height: thumbnailGridView.cellHeight

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            thumbnailGridItem.select();
                        }
                    }
                    function select() {
                        thumbnailGridView.currentIndex = index;
                        thumbnailGridView.currentIndexChanged(thumbnailGridView.currentIndex);
                    }

                    ColumnLayout {
                        z: 0
                        spacing: 0
                        anchors.fill: parent
                        anchors.leftMargin: hoverItem.margins.left
                        anchors.topMargin: hoverItem.margins.top
                        anchors.rightMargin: hoverItem.margins.right
                        anchors.bottomMargin: hoverItem.margins.bottom

                        RowLayout {
                            id: captionRow
                            spacing: units.smallSpacing

                            QIconItem {
                                id: iconItem
                                Layout.minimumHeight: thumbnailGridView.iconSize
                                Layout.minimumWidth: thumbnailGridView.iconSize
                                Layout.maximumHeight: Layout.minimumHeight
                                Layout.maximumWidth: Layout.minimumWidth
                                icon: model.icon
                                state: index == thumbnailGridView.currentIndex ? QIconItem.ActiveState : QIconItem.DefaultState
                            }

                            PlasmaComponents3.Label {
                                id: label
                                Layout.fillWidth: true
                                text: model.caption
                                elide: Text.ElideRight
                            }

                            PlasmaComponents3.ToolButton {
                                id: closeButton
                                visible: model.closeable && typeof tabBox.model.close !== 'undefined' || false
                                icon.name: 'window-close-symbolic'
                                onClicked: {
                                    tabBox.model.close(index);
                                }
                            }
                        }

                        // KWin.ThumbnailItem needs a container
                        // otherwise it will be drawn the same size as the parent ColumnLayout
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            // Cannot draw anything (like an icon) on top of thumbnail
                            KWin.ThumbnailItem {
                                id: thumbnailItem
                                anchors.fill: parent
                                wId: windowId
                                clip: true
                                clipTo: parent
                            }
                        }
                    }
                } // GridView.delegate

                highlight: PlasmaCore.FrameSvgItem {
                    id: highlightItem
                    imagePath: "widgets/viewitem"
                    prefix: "hover"
                }

                Connections {
                    target: tabBox
                    function onCurrentIndexChanged() {
                        thumbnailGridView.currentIndex = tabBox.currentIndex;
                    }
                }
            } // GridView

            Keys.onPressed: {
                if (event.key == Qt.Key_Left) {
                    thumbnailGridView.moveCurrentIndexLeft();
                } else if (event.key == Qt.Key_Right) {
                    thumbnailGridView.moveCurrentIndexRight();
                } else if (event.key == Qt.Key_Up) {
                    thumbnailGridView.moveCurrentIndexUp();
                } else if (event.key == Qt.Key_Down) {
                    thumbnailGridView.moveCurrentIndexDown();
                } else {
                    return;
                }

                thumbnailGridView.currentIndexChanged(thumbnailGridView.currentIndex);
            }
        } // Dialog.mainItem
    } // Dialog
}
