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
    property var screenSize: Qt.size(Screen.width, Screen.height)

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
    property int cfg_DynamicMode: 0
    property bool cfg_Geolocation: false

    signal configurationChanged()
    /**
     * Emitted when the user finishes adding images using the file dialog.
     */
    signal wallpaperBrowseCompleted();

    onScreenSizeChanged: function() {
        if (thumbnailsLoader.item) {
            thumbnailsLoader.item.screenSize = root.screenSize;
        }
    }

    function saveConfig() {
        if (configDialog.currentWallpaper === "org.kde.image") {
            imageWallpaper.wallpaperModel.commitAddition();
            imageWallpaper.wallpaperModel.commitDeletion();
        }
        wallpaperConfiguration.PreviewImage = "null"; // internal, no need to save to file
    }

    function openChooserDialog() {
        const dialogComponent = Qt.createComponent("AddFileDialog.qml");
        dialogComponent.createObject(root);
        dialogComponent.destroy();
    }

    function selectWallpaper(wallpaper: string, selectors: list<string>): void {
        let selector = "";
        if (selectors.includes("day-night")) {
            if (cfg_DynamicMode == 1) {
                selector = "day-night";
            }
        }

        cfg_Image = PlasmaWallpaper.WallpaperUrl.make(wallpaper, selector);
        wallpaperConfiguration.PreviewImage = cfg_Image;
    }

    function selectDynamicMode(mode: int): void {
        cfg_DynamicMode = mode;

        selectWallpaper(thumbnailsLoader.item.view.currentItem.key,
                        thumbnailsLoader.item.view.currentItem.selectors);
    }

    PlasmaWallpaper.ImageBackend {
        id: imageWallpaper
        renderingMode: (configDialog.currentWallpaper === "org.kde.image") ? PlasmaWallpaper.ImageBackend.SingleImage : PlasmaWallpaper.ImageBackend.SlideShow
        targetSize: {
            // Lock screen configuration case
            return Qt.size(root.screenSize.width * Screen.devicePixelRatio, root.screenSize.height * Screen.devicePixelRatio)
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

    spacing: 0

    Kirigami.FormLayout {
        id: formLayout

        Layout.bottomMargin: configDialog.currentWallpaper === "org.kde.image" ? Kirigami.Units.largeSpacing : 0

        Component.onCompleted: function() {
            if (typeof appearanceRoot !== "undefined") {
                twinFormLayouts.push(appearanceRoot.parentLayout);
            }
        }

        QtControls2.ComboBox {
            id: resizeComboBox
            Kirigami.FormData.label: i18ndc("plasma_wallpaper_org.kde.image", "@label:listbox", "Positioning:")
            model: [
                        {
                            'label': i18ndc("plasma_wallpaper_org.kde.image", "@item:inlistbox", "Scaled and cropped"),
                            'fillMode': Image.PreserveAspectCrop
                        },
                        {
                            'label': i18ndc("plasma_wallpaper_org.kde.image", "@item:inlistbox", "Scaled"),
                            'fillMode': Image.Stretch
                        },
                        {
                            'label': i18ndc("plasma_wallpaper_org.kde.image", "@item:inlistbox", "Scaled, keep proportions"),
                            'fillMode': Image.PreserveAspectFit
                        },
                        {
                            'label': i18ndc("plasma_wallpaper_org.kde.image", "@item:inlistbox", "Centered"),
                            'fillMode': Image.Pad
                        },
                        {
                            'label': i18ndc("plasma_wallpaper_org.kde.image", "@item:inlistbox", "Tiled"),
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

        QtControls2.ButtonGroup { id: dayNightModeGroup }

        QtControls2.RadioButton {
            Kirigami.FormData.label: i18ndc("plasma_wallpaper_org.kde.image", "part of a sentence: 'Switch dynamic wallpapers [based on]'", "Switch dynamic wallpapers:")
            text: i18ndc("plasma_wallpaper_org.kde.image", "part of a sentence: 'Switch dynamic wallpapers'", "Based on whether the color scheme is dark or light")
            QtControls2.ButtonGroup.group: dayNightModeGroup
            checked: cfg_DynamicMode === 0
            onToggled: selectDynamicMode(0)
        }

        QtControls2.RadioButton {
            id: dayNightTimeOfDayButton
            text: i18ndc("plasma_wallpaper_org.kde.image", "part of a sentence: 'Switch dynamic wallpapers'", "At sunrise and sunset")
            QtControls2.ButtonGroup.group: dayNightModeGroup
            checked: cfg_DynamicMode === 1
            onToggled: selectDynamicMode(1)
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            Item {
                Layout.preferredWidth: Kirigami.Units.gridUnit
            }

            QtControls2.CheckBox {
                enabled: dayNightTimeOfDayButton.checked
                text: i18ndc("plasma_wallpaper_org.kde.image", "@label:checkbox Use geolocation services for more accurate sunset and sunrise times", "Use device's current location for more accurate timings")
                checked: cfg_Geolocation
                onToggled: cfg_Geolocation = !cfg_Geolocation
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
