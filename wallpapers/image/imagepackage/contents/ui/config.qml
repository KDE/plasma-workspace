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
import org.kde.config as KConfig

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
    property list<string> cfg_SlidePaths: []
    property list<string> cfg_SlidePathsDefault: []
    property int cfg_SlideInterval: 0
    property int cfg_SlideIntervalDefault: 0
    property list<string> cfg_UncheckedSlides: []
    property list<string> cfg_UncheckedSlidesDefault: []
    property int cfg_DynamicMode: 0
    property int cfg_DynamicModeDefault: 0
    property bool cfg_ForceImageAnimation: false
    property bool cfg_ForceImageAnimationDefault: false

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
        cfg_Image = imageWallpaper.makeWallpaperUrl(wallpaper, selectors);
        wallpaperConfiguration.PreviewImage = cfg_Image;
    }

    function selectDynamicMode(mode: /*PlasmaWallpaper.DynamicMode*/ int): void {
        cfg_DynamicMode = mode;

        if (root.configDialog.currentWallpaper === "org.kde.image") {
            selectWallpaper(thumbnailsLoader.item.view.currentItem.key,
                            thumbnailsLoader.item.view.currentItem.selectors);
        }
    }

    PlasmaWallpaper.ImageBackend {
        id: imageWallpaper
        renderingMode: (root.configDialog.currentWallpaper === "org.kde.image") ? PlasmaWallpaper.ImageBackend.SingleImage : PlasmaWallpaper.ImageBackend.SlideShow
        targetSize: {
            // Lock screen configuration case
            return Qt.size(root.screenSize.width * Screen.devicePixelRatio, root.screenSize.height * Screen.devicePixelRatio)
        }
        dynamicMode: root.cfg_DynamicMode
        onSlidePathsChanged: root.cfg_SlidePaths = slidePaths
        onUncheckedSlidesChanged: root.cfg_UncheckedSlides = uncheckedSlides
        onSlideshowModeChanged: root.cfg_SlideshowMode = slideshowMode
        onSlideshowFoldersFirstChanged: root.cfg_SlideshowFoldersFirst = slideshowFoldersFirst

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

        Layout.bottomMargin: root.configDialog.currentWallpaper === "org.kde.image" ? Kirigami.Units.largeSpacing : 0

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
            ]

            textRole: "label"
            onActivated: root.cfg_FillMode = model[currentIndex]["fillMode"]
            Component.onCompleted: setMethod();

            KCM.SettingHighlighter {
                highlight: root.cfg_FillModeDefault != root.cfg_FillMode
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


        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Kirigami.FormData.label: i18ndc("plasma_wallpaper_org.kde.image", "@label:listbox part of a sentence: 'Switch dynamic wallpapers [based on]'", "Switch dynamic wallpapers:")

            QtControls2.ComboBox {
                valueRole: "dynamicMode"
                textRole: "text"
                model: [
                    {
                        dynamicMode: PlasmaWallpaper.DynamicMode.Automatic,
                        text: i18ndc("plasma_wallpaper_org.kde.image", "@item:inlistbox part of a sentence: 'Switch dynamic wallpapers'", "Based on whether the Plasma style is light or dark ")},
                    {
                        dynamicMode: PlasmaWallpaper.DynamicMode.DayNight, text: i18ndc("plasma_wallpaper_org.kde.image", "@item:inlistbox part of a sentence: 'Switch dynamic wallpapers'", "Based on the day-night cycle")
                    },
                    {
                        dynamicMode: PlasmaWallpaper.DynamicMode.AlwaysLight,
                        text: i18ndc("plasma_wallpaper_org.kde.image", "@item:inlistbox", "Always use light variant")
                    },
                    {
                        dynamicMode: PlasmaWallpaper.DynamicMode.AlwaysDark,
                        text: i18ndc("plasma_wallpaper_org.kde.image", "@item:inlistbox", "Always use dark variant")
                    }
                ]
                onActivated: root.selectDynamicMode(currentValue)
                Component.onCompleted: currentIndex = indexOfValue(root.cfg_DynamicMode)

                KCM.SettingHighlighter {
                    highlight: root.cfg_DynamicModeDefault !== root.cfg_DynamicMode
                }
            }

            QtControls2.Button {
                visible: root.cfg_DynamicMode == 1
                enabled: KConfig.KAuthorized.authorizeControlModule("kcm_nighttime")
                text: i18nc("@action:button Configure day-night cycle times", "Configure…")
                icon.name: "configure"
                onClicked: KCM.KCMLauncher.open("kcm_nighttime")
            }
        }

        QtControls2.ButtonGroup { id: backgroundGroup }

        QtControls2.RadioButton {
            id: blurRadioButton
            visible: root.cfg_FillMode === Image.PreserveAspectFit || root.cfg_FillMode === Image.Pad
            Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.image", "Background:")
            text: i18nd("plasma_wallpaper_org.kde.image", "Blur")
            QtControls2.ButtonGroup.group: backgroundGroup
        }

        RowLayout {
            id: colorRow
            visible: root.cfg_FillMode === Image.PreserveAspectFit || root.cfg_FillMode === Image.Pad
            QtControls2.RadioButton {
                id: colorRadioButton
                text: i18nd("plasma_wallpaper_org.kde.image", "Solid color")
                checked: !root.cfg_Blur
                QtControls2.ButtonGroup.group: backgroundGroup

                KCM.SettingHighlighter {
                    highlight: root.cfg_Blur != root.cfg_BlurDefault
                }
            }
            KQuickControls.ColorButton {
                id: colorButton
                color: root.cfg_Color
                dialogTitle: i18nd("plasma_wallpaper_org.kde.image", "Select Background Color")

                KCM.SettingHighlighter {
                    highlight: root.cfg_Color != root.cfg_ColorDefault
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
                if (root.configDialog.currentWallpaper === "org.kde.image") {
                    imageWallpaper.addUsersWallpaper(url);
                } else {
                    imageWallpaper.addSlidePath(url);
                }
            });
            // Scroll to top to view added images
            if (root.configDialog.currentWallpaper === "org.kde.image") {
                thumbnailsLoader.item.view.positionViewAtIndex(0, GridView.Beginning);
            }
        }

        Loader {
            id: thumbnailsLoader
            anchors.fill: parent

            function loadWallpaper () {
                let source = (root.configDialog.currentWallpaper == "org.kde.image") ? "ThumbnailsComponent.qml" :
                    ((root.configDialog.currentWallpaper == "org.kde.slideshow") ? "SlideshowComponent.qml" : "");

                let props = {screenSize: root.screenSize};

                if (root.configDialog.currentWallpaper == "org.kde.slideshow") {
                    props["configuration"] = root.wallpaperConfiguration;
                }
                thumbnailsLoader.setSource(source, props);
            }
        }

        Connections {
            target: root.configDialog
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
