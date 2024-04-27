/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.newstuff as NewStuff

Item {
    id: thumbnailsComponent
    anchors.fill: parent

    property alias view: wallpapersGrid.view
    property var screenSize: Qt.size(Screen.width, Screen.height)

    readonly property QtObject imageModel: (configDialog.currentWallpaper === "org.kde.image") ? imageWallpaper.wallpaperModel : imageWallpaper.slideFilterModel

    Connections {
        target: imageWallpaper
        function onLoadingChanged(loading: bool) {
            if (loading) {
                return;
            }
            if (configDialog.currentWallpaper === "org.kde.image" && imageModel.indexOf(cfg_Image) < 0) {
                imageWallpaper.addUsersWallpaper(cfg_Image);
            }
            wallpapersGrid.resetCurrentIndex();
        }
    }

    Connections {
        target: root
        function onWallpaperBrowseCompleted() {
            // Scroll to top to view added images
            wallpapersGrid.view.positionViewAtIndex(0, GridView.Beginning);
            wallpapersGrid.resetCurrentIndex(); // BUG 455129
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // FIXME: can't make it a header of the grid view due to the lack of a
        // headerPositioning: property; see https://bugreports.qt.io/browse/QTBUG-117035.
        Kirigami.InlineViewHeader {
            Layout.fillWidth: true
            text: i18nd("plasma_wallpaper_org.kde.image", "Images")
            actions: [
                Kirigami.Action {
                    icon.name: "insert-image-symbolic"
                    text: i18ndc("plasma_wallpaper_org.kde.image", "@action:button the thing being added is an image file", "Add…")
                    visible: configDialog.currentWallpaper == "org.kde.image"
                    onTriggered: root.openChooserDialog();
                },
                NewStuff.Action {
                    configFile: Kirigami.Settings.isMobile ? "wallpaper-mobile.knsrc" : "wallpaper.knsrc"
                    text: i18ndc("plasma_wallpaper_org.kde.image", "@action:button the new things being gotten are wallpapers", "Get New…")
                    viewMode: NewStuff.Page.ViewMode.Preview
                }
            ]
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            color: Kirigami.Theme.backgroundColor

            KCM.GridView {
                id: wallpapersGrid
                anchors.fill: parent

                framedView: false


                function resetCurrentIndex() {
                    //that min is needed as the module will be populated in an async way
                    //and only on demand so we can't ensure it already exists
                    if (configDialog.currentWallpaper === "org.kde.image") {
                        wallpapersGrid.view.currentIndex = Qt.binding(() => configDialog.currentWallpaper === "org.kde.image" ?  Math.min(imageModel.indexOf(cfg_Image), imageModel.count - 1) : 0);
                    }
                }

                //kill the space for label under thumbnails
                view.model: thumbnailsComponent.imageModel

                //set the size of the cell, depending on Screen resolution to respect the aspect ratio
                view.implicitCellWidth: screenSize.width / 10 + Kirigami.Units.smallSpacing * 2
                view.implicitCellHeight: screenSize.height / 10 + Kirigami.Units.smallSpacing * 2 + Kirigami.Units.gridUnit * 3

                view.reuseItems: true

                view.delegate: WallpaperDelegate {
                    color: cfg_Color
                }
            }
        }
    }

    KCM.SettingHighlighter {
        target: wallpapersGrid
        highlight: configDialog.currentWallpaper === "org.kde.image" && cfg_Image != cfg_ImageDefault
    }
}
