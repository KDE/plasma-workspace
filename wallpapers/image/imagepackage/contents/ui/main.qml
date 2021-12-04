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

    readonly property string modelImage: imageWallpaper.wallpaperPath
    readonly property string configuredImage: wallpaper.configuration.Image
    readonly property int fillMode: wallpaper.configuration.FillMode
    readonly property string configColor: wallpaper.configuration.Color
    readonly property bool blur: wallpaper.configuration.Blur
    readonly property size sourceSize: Qt.size(root.width * Screen.devicePixelRatio, root.height * Screen.devicePixelRatio)

    // Ppublic API functions accessible from C++:

    // e.g. used by WallpaperInterface for drag and drop
    function setUrl(url) {
        wallpaper.configuration.Image = url
        imageWallpaper.addUsersWallpaper(url);
    }

    // e.g. used by slideshow wallpaper plugin
    function action_next() {
        imageWallpaper.nextSlide();
    }

    // e.g. used by slideshow wallpaper plugin
    function action_open() {
        Qt.openUrlExternally(modelImage)
    }

    //private

    onConfiguredImageChanged: {
        if (modelImage != configuredImage && configuredImage != "") {
            imageWallpaper.addUrl(configuredImage);
        }
    }

    Connections {
        target: wallpaper
        function onSlideShowClosed() {
            wallpaper.removeAction("open");
            wallpaper.removeAction("next");
        }
    }

    Component.onCompleted: {
        wallpaper.loading = true; // delays ksplash until the wallpaper has been loaded

        if (wallpaper.pluginName === "org.kde.slideshow") {
            // NOTE: Actions added here need to be removed in onSlideShowClosed()
            wallpaper.setAction("open", i18nd("plasma_wallpaper_org.kde.image", "Open Wallpaper Image"), "document-open");
            wallpaper.setAction("next", i18nd("plasma_wallpaper_org.kde.image", "Next Wallpaper Image"), "user-desktop");
        }
    }

    Wallpaper.Image {
        id: imageWallpaper
        //the oneliner of difference between image and slideshow wallpapers
        renderingMode: (wallpaper.pluginName === "org.kde.image") ? Wallpaper.Image.SingleImage : Wallpaper.Image.SlideShow
        targetSize: root.sourceSize
        slidePaths: wallpaper.configuration.SlidePaths
        slideTimer: wallpaper.configuration.SlideInterval
        slideshowMode: wallpaper.configuration.SlideshowMode
        slideshowFoldersFirst: wallpaper.configuration.SlideshowFoldersFirst
        uncheckedSlides: wallpaper.configuration.UncheckedSlides
    }

    onFillModeChanged: Qt.callLater(loadImage);
    onModelImageChanged:{
        Qt.callLater(loadImage);
        wallpaper.configuration.Image = modelImage;
    }
    onConfigColorChanged: Qt.callLater(loadImage);
    onBlurChanged: Qt.callLater(loadImage);
    onWidthChanged: Qt.callLater(loadImage);
    onHeightChanged: Qt.callLater(loadImage);

    function loadImage() {
        var isFirst = (root.currentItem == undefined);
        var pendingImage = baseImage.createObject(root, { "source": root.modelImage,
                        "fillMode": root.fillMode,
                        "sourceSize": root.sourceSize,
                        "color": root.configColor,
                        "blur": root.blur,
                        "opacity": isFirst ? 1: 0});

        function replaceWhenLoaded() {
            if (pendingImage.status !== Image.Loading) {
                root.replace(pendingImage, {},
                    isFirst ? QQC2.StackView.Immediate : QQC2.StackView.Transition);//dont' animate first show
                pendingImage.statusChanged.disconnect(replaceWhenLoaded);

                wallpaper.loading = false;
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
            from: 0
            to: 1
            duration: wallpaper.configuration.TransitionAnimationDuration
        }
    }
    // Keep the old image around till the new one is fully faded in
    // If we fade both at the same time you can see the background behind glimpse through
    replaceExit: Transition{
        PauseAnimation {
            duration: wallpaper.configuration.TransitionAnimationDuration
        }
    }
}
