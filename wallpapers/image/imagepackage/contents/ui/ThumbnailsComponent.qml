/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Window 2.15 // for Screen

import org.kde.kcm 1.5 as KCM
import org.kde.kirigami 2.12 as Kirigami
import org.kde.plasma.plasmoid 2.0

Item {
    id: thumbnailsComponent
    anchors.fill: parent

    property alias view: wallpapersGrid.view

    readonly property var imageModel: (configDialog.currentWallpaper === "org.kde.image") ? imageWallpaper.wallpaperModel : imageWallpaper.slideFilterModel

    Connections {
        target: imageWallpaper
        function onLoadingChanged() {
            if (imageWallpaper.loading) {
                return;
            }
            if (configDialog.currentWallpaper === "org.kde.image" && imageModel.indexOf(cfg_Image) < 0) {
                imageWallpaper.addUsersWallpaper(cfg_Image);
            }
            wallpapersGrid.resetCurrentIndex();
        }

        function onWallpaperBrowseCompleted() {
            // Scroll to top to view added images
            wallpapersGrid.view.positionViewAtIndex(0, GridView.Beginning);
            wallpapersGrid.resetCurrentIndex(); // BUG 455129
        }
    }

    KCM.GridView {
        id: wallpapersGrid
        anchors.fill: parent

        function resetCurrentIndex() {
            //that min is needed as the module will be populated in an async way
            //and only on demand so we can't ensure it already exists
            if (configDialog.currentWallpaper === "org.kde.image") {
                view.currentIndex = Qt.binding(() =>  Math.min(imageModel.indexOf(cfg_Image), imageModel.count - 1));
            }
        }

        //kill the space for label under thumbnails
        view.model: thumbnailsComponent.imageModel
        Component.onCompleted: {
            thumbnailsComponent.imageModel.usedInConfig = true;
        }

        //set the size of the cell, depending on Screen resolution to respect the aspect ratio
        view.implicitCellWidth: {
            let screenWidth = 0;
            if (typeof Plasmoid !== "undefined") {
                screenWidth = Plasmoid.width;
            } else {
                screenWidth = Screen.width;
            }

            return screenWidth / 10 + Kirigami.Units.smallSpacing * 2
        }
        view.implicitCellHeight: {
            let screenHeight = 0;
            if (typeof Plasmoid !== "undefined") {
                screenHeight = Plasmoid.height;
            } else {
                screenHeight = Screen.height;
            }

            return screenHeight / 10 + Kirigami.Units.smallSpacing * 2 + Kirigami.Units.gridUnit * 3
        }

        view.reuseItems: true

        view.delegate: WallpaperDelegate {
            color: cfg_Color
        }
    }

    KCM.SettingHighlighter {
        target: wallpapersGrid
        highlight: configDialog.currentWallpaper === "org.kde.image" && cfg_Image != cfg_ImageDefault
    }
}
