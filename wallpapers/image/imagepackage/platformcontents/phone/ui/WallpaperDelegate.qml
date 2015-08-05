/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *  Copyright 2014 Sebastian Kügler <sebas@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Controls.Private 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents

MouseArea {
    id: wallpaperDelegate

    width: wallpapersGrid.cellWidth
    height: wallpapersGrid.cellHeight

    property bool selected: (wallpapersGrid.currentIndex == index)

    onSelectedChanged: {
        cfg_Image = model.path
    }

    hoverEnabled: true


    //note: this *doesn't* use system colors since it represent a 
    //skeymorphic photograph rather than a widget
    Rectangle {
        id: background
        color: "white"
        anchors {
            fill: parent
            margins: units.smallSpacing
        }
        opacity: 0.8
        Rectangle {
            color: cfg_Color
            anchors {
                fill: parent
                margins: units.smallSpacing * 2
            }
            QIconItem {
                anchors.centerIn: parent
                width: units.iconSizes.large
                height: width
                icon: "view-preview"
                visible: !walliePreview.visible
            }
            QPixmapItem {
                id: walliePreview
                anchors.fill: parent
                visible: model.screenshot != null
                smooth: true
                pixmap: model.screenshot
                fillMode: {
                    if (cfg_FillMode == Image.Stretch) {
                        return QPixmapItem.Stretch;
                    } else if (cfg_FillMode == Image.PreserveAspectFit) {
                        return QPixmapItem.PreserveAspectFit;
                    } else if (cfg_FillMode == Image.PreserveAspectCrop) {
                        return QPixmapItem.PreserveAspectCrop;
                    } else if (cfg_FillMode == Image.Tile) {
                        return QPixmapItem.Tile;
                    } else if (cfg_FillMode == Image.TileVertically) {
                        return QPixmapItem.TileVertically;
                    } else if (cfg_FillMode == Image.TileHorizontally) {
                        return QPixmapItem.TileHorizontally;
                    }
                    return QPixmapItem.Pad;
                }
            }
            PlasmaComponents.ToolButton {
                anchors {
                    top: parent.top
                    right: parent.right
                    margins: units.smallSpacing
                }
                iconSource: "list-remove"
                tooltip: i18nd("plasma_applet_org.kde.image", "Remove wallpaper")
                flat: false
                visible: model.removable
                onClicked: imageWallpaper.removeWallpaper(model.packageName)
            }
        }
    }

    Rectangle {
        opacity: selected ? 1.0 : 0
        anchors.fill: background
        border.width: units.smallSpacing * 2
        border.color: syspal.highlight
        color: "transparent"
        Behavior on opacity {
            PropertyAnimation {
                duration: units.longDuration
                easing.type: Easing.OutQuad
            }
        }
    }


    Timer {
        interval: 1000 // FIXME TODO: Use platform value for tooltip activation delay.

        running: wallpaperDelegate.containsMouse && !pressed && model.display && model.author

        onTriggered: {
            Tooltip.showText(wallpaperDelegate, Qt.point(wallpaperDelegate.mouseX, wallpaperDelegate.mouseY),
                i18nd("plasma_applet_org.kde.image", "%1 by %2", model.display, model.author));
        }
    }

    onClicked: {
        wallpapersGrid.currentIndex = index
        cfg_Image = model.path
    }

    onExited: Tooltip.hideText()

    Component.onCompleted: {
        if (cfg_Image == model.path) {
            makeCurrentTimer.pendingIndex = model.index
            makeCurrentTimer.restart()
        }
    }
}
