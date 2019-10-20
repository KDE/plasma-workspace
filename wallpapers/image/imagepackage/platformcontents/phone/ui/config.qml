/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
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

//We need units from it
import org.kde.plasma.core 2.0 as Plasmacore
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.kquickcontrolsaddons 2.0
import QtQuick.Controls 2.8 as QQC2

Item {
    id: root
    anchors {
        top: parent.top
        left: parent.left
        right: parent.right
        bottom: parent.bottom
    }
    property string cfg_Image

    Wallpaper.Image {
        id: imageWallpaper
        targetSize: Qt.size(plasmoid.width, plasmoid.height)
    }

    //Rectangle { color: "orange"; x: formAlignment; width: formAlignment; height: 20 }

    QQC2.ScrollView {
        anchors.fill: parent

        frameVisible: true

        GridView {
            id: wallpapersGrid
            model: imageWallpaper.wallpaperModel
            currentIndex: -1

            cellWidth: Math.floor(wallpapersGrid.width / Math.max(Math.floor(wallpapersGrid.width / (units.gridUnit*12)), 3))
            cellHeight: cellWidth / (plasmoid.width / plasmoid.height)

            anchors.margins: 4
            boundsBehavior: Flickable.DragAndOvershootBounds

            delegate: WallpaperDelegate {}

            onCountChanged: {
                wallpapersGrid.currentIndex = imageWallpaper.wallpaperModel.indexOf(cfg_Image);
                wallpapersGrid.positionViewAtIndex(wallpapersGrid.currentIndex, GridView.Visible)
            }

            Connections {
                target: imageWallpaper
                onCustomWallpaperPicked: wallpapersGrid.currentIndex = 0
            }
        }
    }

    QQC2.Button {
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        iconName: "list-add"
        text: i18nd("plasma_wallpaper_org.kde.image","Add Custom Wallpaper")
        onClicked: customWallpaperLoader.source = Qt.resolvedUrl("customwallpaper.qml")
    }

    Loader {
        id: customWallpaperLoader
        anchors.fill: parent
    }
}
