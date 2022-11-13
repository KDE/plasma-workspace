/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Controls 2.5 as QtControls2
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0 // for Screen
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.wallpapers.image 2.0 as PlasmaWallpaper
import org.kde.kquickcontrols 2.0 as KQuickControls
import org.kde.kquickcontrolsaddons 2.0
import org.kde.newstuff 1.91 as NewStuff
import org.kde.kcm 1.5 as KCM
import org.kde.kirigami 2.12 as Kirigami

ColumnLayout {
    id: root
    property alias cfg_Color: colorButton.color
    property color cfg_ColorDefault
    property string cfg_Image
    property string cfg_ImageDefault
    property int cfg_FillMode
    property int cfg_FillModeDefault
    property int cfg_SlideshowMode
    property int cfg_SlideshowModeDefault
    property bool cfg_SlideshowFoldersFirst
    property bool cfg_SlideshowFoldersFirstDefault: false
    property alias cfg_Blur: blurRadioButton.checked
    property bool cfg_BlurDefault
    property var cfg_SlidePaths: []
    property var cfg_SlidePathsDefault: []
    property int cfg_SlideInterval: 0
    property int cfg_SlideIntervalDefault: 0
    property var cfg_UncheckedSlides: []
    property var cfg_UncheckedSlidesDefault: []

    signal configurationChanged()

    function saveConfig() {
        if (configDialog.currentWallpaper === "org.kde.image") {
            imageWallpaper.wallpaperModel.commitAddition();
            imageWallpaper.wallpaperModel.commitDeletion();
        }
    }

    PlasmaWallpaper.ImageBackend {
        id: imageWallpaper
        renderingMode: (configDialog.currentWallpaper === "org.kde.image") ? PlasmaWallpaper.ImageBackend.SingleImage : PlasmaWallpaper.ImageBackend.SlideShow
        targetSize: {
            if (typeof Plasmoid !== "undefined") {
                return Qt.size(Plasmoid.width * Screen.devicePixelRatio, Plasmoid.height * Screen.devicePixelRatio)
            }
            // Lock screen configuration case
            return Qt.size(Screen.width * Screen.devicePixelRatio, Screen.height * Screen.devicePixelRatio)
        }
        onSlidePathsChanged: cfg_SlidePaths = slidePaths
        onUncheckedSlidesChanged: cfg_UncheckedSlides = uncheckedSlides
        onSlideshowModeChanged: cfg_SlideshowMode = slideshowMode
        onSlideshowFoldersFirstChanged: cfg_SlideshowFoldersFirst = slideshowFoldersFirst

        onSettingsChanged: root.configurationChanged()
    }

    onCfg_FillModeChanged: {
        resizeComboBox.setMethod()
    }

    onCfg_SlidePathsChanged: {
        imageWallpaper.slidePaths = cfg_SlidePaths
    }
    onCfg_UncheckedSlidesChanged: {
        imageWallpaper.uncheckedSlides = cfg_UncheckedSlides
    }

    onCfg_SlideshowModeChanged: {
        imageWallpaper.slideshowMode = cfg_SlideshowMode
    }

    onCfg_SlideshowFoldersFirstChanged: {
        imageWallpaper.slideshowFoldersFirst = cfg_SlideshowFoldersFirst
    }

    //Rectangle { color: "orange"; x: formAlignment; width: formAlignment; height: 20 }

    Kirigami.FormLayout {
        twinFormLayouts: parentLayout
        QtControls2.ComboBox {
            id: resizeComboBox
            Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.image", "Positioning:")
            model: [
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image", "Scaled and Cropped"),
                            'fillMode': Image.PreserveAspectCrop
                        },
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image", "Scaled"),
                            'fillMode': Image.Stretch
                        },
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image", "Scaled, Keep Proportions"),
                            'fillMode': Image.PreserveAspectFit
                        },
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image", "Centered"),
                            'fillMode': Image.Pad
                        },
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image", "Tiled"),
                            'fillMode': Image.Tile
                        }
            ]

            textRole: "label"
            onActivated: cfg_FillMode = model[currentIndex]["fillMode"]
            Component.onCompleted: setMethod();

            KCM.SettingHighlighter {
                highlight: cfg_FillModeDefault != cfg_FillMode
            }

            function setMethod() {
                for (var i = 0; i < model.length; i++) {
                    if (model[i]["fillMode"] === root.cfg_FillMode) {
                        resizeComboBox.currentIndex = i;
                        break;
                    }
                }
            }
        }

        QtControls2.ButtonGroup { id: backgroundGroup }

        QtControls2.RadioButton {
            id: blurRadioButton
            visible: cfg_FillMode === Image.PreserveAspectFit || cfg_FillMode === Image.Pad
            Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.image", "Background:")
            text: i18nd("plasma_wallpaper_org.kde.image", "Blur")
            QtControls2.ButtonGroup.group: backgroundGroup
        }

        RowLayout {
            id: colorRow
            visible: cfg_FillMode === Image.PreserveAspectFit || cfg_FillMode === Image.Pad
            QtControls2.RadioButton {
                id: colorRadioButton
                text: i18nd("plasma_wallpaper_org.kde.image", "Solid color")
                checked: !cfg_Blur
                QtControls2.ButtonGroup.group: backgroundGroup

                KCM.SettingHighlighter {
                    highlight: cfg_Blur != cfg_BlurDefault
                }
            }
            KQuickControls.ColorButton {
                id: colorButton
                dialogTitle: i18nd("plasma_wallpaper_org.kde.image", "Select Background Color")

                KCM.SettingHighlighter {
                    highlight: cfg_Color != cfg_ColorDefault
                }
            }
        }
    }

    DropArea {
        Layout.fillWidth: true
        Layout.fillHeight: true

        onEntered: {
            if (drag.hasUrls) {
                drag.accept();
            }
        }
        onDropped: {
            drop.urls.forEach(function (url) {
                if (configDialog.currentWallpaper === "org.kde.image") {
                    imageWallpaper.addUsersWallpaper(url);
                } else {
                    imageWallpaper.addSlidePath(url);
                }
            });
            // Scroll to top to view added images
            if (configDialog.currentWallpaper === "org.kde.image") {
                thumbnailsLoader.item.view.positionViewAtIndex(0, GridView.Beginning);
            }
        }

        Loader {
            id: thumbnailsLoader
            anchors.fill: parent
            source: (configDialog.currentWallpaper == "org.kde.image") ? "ThumbnailsComponent.qml" :
                ((configDialog.currentWallpaper == "org.kde.slideshow") ? "SlideshowComponent.qml" : "")
        }
    }

    RowLayout {
        id: buttonsRow
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        visible: configDialog.currentWallpaper == "org.kde.image"
        QtControls2.Button {
            icon.name: "list-add"
            text: i18nd("plasma_wallpaper_org.kde.image","Add Image…")
            onClicked: imageWallpaper.showFileDialog();
        }
        NewStuff.Button {
            Layout.alignment: Qt.AlignRight
            configFile: Kirigami.Settings.isMobile ? "wallpaper-mobile.knsrc" : "wallpaper.knsrc"
            text: i18nd("plasma_wallpaper_org.kde.image", "Get New Wallpapers…")
            viewMode: NewStuff.Page.ViewMode.Preview
        }
    }

    Component.onDestruction: {
        wallpaper.configuration.PreviewImage = "null";
    }
}
