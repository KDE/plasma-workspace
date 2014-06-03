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
import org.kde.plasma.mobilecomponents 0.2 as MobileComponets

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
        width: wallpaper.configuration.width
        height: wallpaper.configuration.height
    }

    //Rectangle { color: "orange"; x: formAlignment; width: formAlignment; height: 20 }

    MobileComponets.IconGrid {
        id: wallpapersGrid
        model: imageWallpaper.wallpaperModel

        property int currentIndex: -1
        onCurrentIndexChanged: {
            currentPage = Math.max(0, Math.floor(currentIndex/pageSize))
        }
        anchors {
            fill: parent
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        delegateWidth: Math.floor(wallpapersGrid.width / Math.max(Math.floor(wallpapersGrid.width / (units.gridUnit*12)), 3))
        delegateHeight: delegateWidth / 1.6

        delegate: WallpaperDelegate {}
        Timer {
            id: makeCurrentTimer
            interval: 100
            repeat: false
            property string pendingIndex
            onTriggered: {
                wallpapersGrid.currentIndex = pendingIndex
                wallpapersGrid.positionViewAtIndex(pendingIndex, ListView.Beginning)
            }
        }

        Connections {
            target: imageWallpaper
                onCustomWallpaperPicked: wallpapersGrid.currentIndex = 0
            }
    }
}
