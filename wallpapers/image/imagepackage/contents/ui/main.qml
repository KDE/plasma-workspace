/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Controls 2.1 as QQC2
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.plasma.core 2.0 as PlasmaCore

QQC2.StackView {
    id: root

    readonly property url modelImage: mediaProxy.modelImage
    readonly property int fillMode: wallpaper.configuration.FillMode
    readonly property string configColor: wallpaper.configuration.Color
    readonly property bool blur: wallpaper.configuration.Blur
    readonly property size sourceSize: Qt.size(root.width * Screen.devicePixelRatio, root.height * Screen.devicePixelRatio)

    // Ppublic API functions accessible from C++:

    // e.g. used by WallpaperInterface for drag and drop
    function setUrl(url) {
        if (wallpaper.pluginName === "org.kde.image") {
            const result = imageWallpaper.addUsersWallpaper(url);

            if (result.length > 0) {
                // Can be a file or a folder (KPackage)
                wallpaper.configuration.Image = result;
            }
        } else {
            imageWallpaper.addSlidePath(url);
            // Save drag and drop result
            wallpaper.configuration.SlidePaths = imageWallpaper.slidePaths;
        }
    }

    // e.g. used by slideshow wallpaper plugin
    function action_next() {
        imageWallpaper.nextSlide();
    }

    // e.g. used by slideshow wallpaper plugin
    function action_open() {
        mediaProxy.openModelImage();
    }

    //private

    Component.onCompleted: {
        // In case plasmashell crashes when the config dialog is opened
        wallpaper.configuration.PreviewImage = "null";
        wallpaper.loading = true; // delays ksplash until the wallpaper has been loaded

        if (wallpaper.pluginName === "org.kde.slideshow") {
            wallpaper.setAction("open", i18nd("plasma_wallpaper_org.kde.image", "Open Wallpaper Image"), "document-open");
            wallpaper.setAction("next", i18nd("plasma_wallpaper_org.kde.image", "Next Wallpaper Image"), "user-desktop");
        }
    }

    Wallpaper.ImageBackend {
        id: imageWallpaper
        usedInConfig: false
        //the oneliner of difference between image and slideshow wallpapers
        renderingMode: (wallpaper.pluginName === "org.kde.image") ? Wallpaper.ImageBackend.SingleImage : Wallpaper.ImageBackend.SlideShow
        targetSize: root.sourceSize
        slidePaths: wallpaper.configuration.SlidePaths
        slideTimer: wallpaper.configuration.SlideInterval
        slideshowMode: wallpaper.configuration.SlideshowMode
        slideshowFoldersFirst: wallpaper.configuration.SlideshowFoldersFirst
        uncheckedSlides: wallpaper.configuration.UncheckedSlides
    }

    Wallpaper.MediaProxy {
        id: mediaProxy

        source: {
            if (wallpaper.pluginName === "org.kde.slideshow") {
                return imageWallpaper.image;
            }
            if (wallpaper.configuration.PreviewImage !== "null") {
                return wallpaper.configuration.PreviewImage;
            }
            return wallpaper.configuration.Image;
        }
        targetSize: root.sourceSize

        onColorSchemeChanged: loadImageImmediately();
    }

    onFillModeChanged: Qt.callLater(loadImage);
    onModelImageChanged: Qt.callLater(loadImage);
    onConfigColorChanged: Qt.callLater(loadImage);
    onBlurChanged: Qt.callLater(loadImage);

    function loadImageImmediately() {
        loadImage(true);
    }

    function loadImage(skipAnimation) {
        const _skipAnimation = root.currentItem == undefined || !!skipAnimation;
        var pendingImage = baseImage.createObject(root, { "source": root.modelImage,
                        "fillMode": root.fillMode,
                        "sourceSize": root.sourceSize,
                        "color": root.configColor,
                        "blur": root.blur,
                        "opacity": _skipAnimation ? 1: 0});

        function replaceWhenLoaded() {
            if (pendingImage.status !== Image.Loading) {
                root.replace(pendingImage, {},
                    _skipAnimation ? QQC2.StackView.Immediate : QQC2.StackView.Transition);
                pendingImage.statusChanged.disconnect(replaceWhenLoaded);

                wallpaper.loading = false;

                if (pendingImage.status !== Image.Ready) {
                    mediaProxy.useSingleImageDefaults();
                }
            }
        }
        pendingImage.statusChanged.connect(replaceWhenLoaded);
        replaceWhenLoaded();
    }

    Component {
        id: baseImage

        Image {
            id: mainImage

            property alias color: backgroundColor.color
            property bool blur: false

            asynchronous: true
            cache: false
            autoTransform: true
            z: -1

            QQC2.StackView.onActivated: {
                // BUG 454908: Update accent color
                wallpaper.repaintNeeded();
            }
            QQC2.StackView.onRemoved: destroy()

            Rectangle {
                id: backgroundColor
                anchors.fill: parent
                visible: mainImage.status === Image.Ready && !blurLoader.active
                z: -2
            }

            Loader {
                id: blurLoader
                anchors.fill: parent
                z: -3
                active: mainImage.blur && (mainImage.fillMode === Image.PreserveAspectFit || mainImage.fillMode === Image.Pad)
                sourceComponent: Item {
                    Image {
                        id: blurSource
                        anchors.fill: parent
                        asynchronous: true
                        cache: false
                        autoTransform: true
                        fillMode: Image.PreserveAspectCrop
                        source: mainImage.source
                        sourceSize: mainImage.sourceSize
                        visible: false // will be rendered by the blur
                    }

                    GaussianBlur {
                        id: blurEffect
                        anchors.fill: parent
                        source: blurSource
                        radius: 32
                        samples: 65
                        visible: blurSource.status === Image.Ready
                    }
                }
            }
        }
    }

    replaceEnter: Transition {
        OpacityAnimator {
            id: replaceEnterOpacityAnimator
            from: 0
            to: 1
            // The value is to keep compatible with the old feeling defined by "TransitionAnimationDuration" (default: 1000)
            duration: Math.round(PlasmaCore.Units.veryLongDuration * 2.5)
        }
    }
    // Keep the old image around till the new one is fully faded in
    // If we fade both at the same time you can see the background behind glimpse through
    replaceExit: Transition{
        PauseAnimation {
            duration: replaceEnterOpacityAnimator.duration
        }
    }
}
