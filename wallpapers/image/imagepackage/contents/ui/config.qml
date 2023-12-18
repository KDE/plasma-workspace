/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QtControls2
import QtQuick.Layouts
import org.kde.plasma.wallpapers.image as PlasmaWallpaper
import org.kde.kquickcontrols as KQuickControls
import org.kde.kquickcontrolsaddons
import org.kde.newstuff as NewStuff
import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

/**
 * For proper alignment, an ancestor **MUST** have id "appearanceRoot" and property "parentLayout"
 */
ColumnLayout {
    id: root
        
    property var configDialog
    property var wallpaperConfiguration: wallpaper.configuration
    property var parentLayout
    property var screen : Screen
    property var screenSize: !!screen.geometry ? Qt.size(screen.geometry.width, screen.geometry.height):  Qt.size(screen.width, screen.height)
    
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
    /**
     * Emitted when the user finishes adding images using the file dialog.
     */
    signal wallpaperBrowseCompleted();
    
    onScreenChanged: function() {
        if (thumbnailsLoader.item) {
            thumbnailsLoader.item.screenSize = !!root.screen.geometry ? Qt.size(root.screen.geometry.width, root.screen.geometry.height):  Qt.size(root.screen.width, root.screen.height);
        }
    }
    
    function saveConfig() {
        if (configDialog.currentWallpaper === "org.kde.image") {
            imageWallpaper.wallpaperModel.commitAddition();
            imageWallpaper.wallpaperModel.commitDeletion();
        }
    }

    function openChooserDialog() {
        const dialogComponent = Qt.createComponent("AddFileDialog.qml");
        dialogComponent.createObject(root);
        dialogComponent.destroy();
    }

    PlasmaWallpaper.ImageBackend {
        id: imageWallpaper
        renderingMode: (configDialog.currentWallpaper === "org.kde.image") ? PlasmaWallpaper.ImageBackend.SingleImage : PlasmaWallpaper.ImageBackend.SlideShow
        targetSize: {
            // Lock screen configuration case
            return Qt.size(root.screenSize.width * root.screen.devicePixelRatio, root.screenSize.height * root.screen.devicePixelRatio)
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
        if (cfg_SlidePaths)
            imageWallpaper.slidePaths = cfg_SlidePaths
    }
    onCfg_UncheckedSlidesChanged: {
        if (cfg_UncheckedSlides)
            imageWallpaper.uncheckedSlides = cfg_UncheckedSlides
    }

    onCfg_SlideshowModeChanged: {
        if (cfg_SlideshowMode)
            imageWallpaper.slideshowMode = cfg_SlideshowMode
    }

    onCfg_SlideshowFoldersFirstChanged: {
        if (cfg_SlideshowFoldersFirst)
            imageWallpaper.slideshowFoldersFirst = cfg_SlideshowFoldersFirst
    }

    //Rectangle { color: "orange"; x: formAlignment; width: formAlignment; height: 20 }

    Kirigami.FormLayout {
        id: formLayout
        
        Component.onCompleted: function() {
            if (typeof appearanceRoot !== "undefined") {
                twinFormLayouts.push(appearanceRoot.parentLayout);
            }
        }
        
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
                color: cfg_Color
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

        onEntered: drag => {
            if (drag.hasUrls) {
                drag.accept();
            }
        }
        onDropped: drop => {
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
        
            function loadWallpaper () {
                let source = (configDialog.currentWallpaper == "org.kde.image") ? "ThumbnailsComponent.qml" :
                    ((configDialog.currentWallpaper == "org.kde.slideshow") ? "SlideshowComponent.qml" : "");
                
                let props = {screenSize: screenSize};
                
                if (configDialog.currentWallpaper == "org.kde.slideshow") {
                    props["configuration"] = wallpaperConfiguration;
                }
                thumbnailsLoader.setSource(source, props);
            }
        }
        
        Connections {
            target: configDialog
            function onCurrentWallpaperChanged() {
                thumbnailsLoader.loadWallpaper();
            }
        }
        
        Component.onCompleted: () => {
            thumbnailsLoader.loadWallpaper();
        }
        
    }

    Component.onDestruction: {
        if (wallpaperConfiguration)
            wallpaperConfiguration.PreviewImage = "null";
    }
}
