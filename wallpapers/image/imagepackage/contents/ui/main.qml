/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Controls 2.1 as QQC2
import QtQuick.Window 2.2
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.plasma.core 2.0 as PlasmaCore

QQC2.StackView {
    id: root

    readonly property url modelImage: mediaProxy.modelImage
    readonly property int fillMode: {
        const mode = wallpaper.configuration.FillMode;
        if (mode == Image.PreserveAspectCrop + 100) {
            return Image.PreserveAspectCrop;
        }
        return mode;
    }
    readonly property string configColor: wallpaper.configuration.Color
    readonly property bool blur: wallpaper.configuration.Blur
    readonly property size sourceSize: Qt.size(root.width * Screen.devicePixelRatio, root.height * Screen.devicePixelRatio)

    readonly property var desktopWindow: Window.window

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

        spanScreens: wallpaper.configuration.FillMode == Image.PreserveAspectCrop + 100
        targetWindow: root.desktopWindow

        onColorSchemeChanged: loadImageImmediately();
        onTargetRectChanged: {
            Qt.callLater(loadImage);
        }
    }

    onFillModeChanged: Qt.callLater(loadImage);
    onModelImageChanged: Qt.callLater(loadImage);
    onConfigColorChanged: Qt.callLater(loadImage);
    onBlurChanged: Qt.callLater(loadImage);

    property Component staticImageComponent
    property Component animatedImageComponent
    property Component videoComponent

    function createBackgroundComponent() {
        switch (mediaProxy.backgroundType) {
        case Wallpaper.BackgroundType.Image: {
            if (!staticImageComponent) {
                staticImageComponent = Qt.createComponent("mediacomponent/StaticImageComponent.qml");
            }
            return staticImageComponent;
        }
        case Wallpaper.BackgroundType.AnimatedImage: {
            if (!animatedImageComponent) {
                animatedImageComponent = Qt.createComponent("mediacomponent/AnimatedImageComponent.qml");
            }
            return animatedImageComponent;
        }
        case Wallpaper.BackgroundType.Video: {
            if (!videoComponent) {
                videoComponent = Qt.createComponent("mediacomponent/VideoComponent.qml");
            }
            return videoComponent;
        }
        }
    }

    function loadImageImmediately() {
        loadImage(true);
    }

    function loadImage(skipAnimation) {
        const _skipAnimation = root.currentItem == undefined || !!skipAnimation;
        const baseImage = createBackgroundComponent();
        var pendingImage = baseImage.createObject(root, {
            // Use mediaProxy instead of root because colorSchemeChanged needs immediately update the wallpaper
            "source": mediaProxy.modelImage,
                        "fillMode": root.fillMode,
                        "sourceSize": root.sourceSize,
                        "color": root.configColor,
                        "blur": root.blur,
            "opacity": _skipAnimation ? 1: 0,
            "width": root.width,
            "height": root.height,
            "spanScreens": mediaProxy.spanScreens,
            "targetRect": mediaProxy.targetRect,
        });

        function replaceWhenLoaded() {
            if (pendingImage.status !== Image.Loading) {
                // BUG 454908: Update accent color
                pendingImage.QQC2.StackView.onActivated.connect(wallpaper.repaintNeeded);
                pendingImage.QQC2.StackView.onRemoved.connect(pendingImage.destroy);
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
