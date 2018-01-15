/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *  Copyright 2014 Sebastian Kügler <sebas@kde.org>
 *  Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
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

import QtQuick 2.5
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    id: root

    readonly property string configuredImage: wallpaper.configuration.Image
    readonly property string modelImage: imageWallpaper.wallpaperPath
    property Item currentImage: imageB
    property Item currentBlurBackground: blurBackgroundB
    property Item otherImage: imageA
    property Item otherBlurBackground: blurBackgroundA
    readonly property int fillMode: wallpaper.configuration.FillMode
    property size sourceSize: Qt.size(root.width * Screen.devicePixelRatio, root.height * Screen.devicePixelRatio)

    //public API, the C++ part will look for those
    function setUrl(url) {
        wallpaper.configuration.Image = url
        imageWallpaper.addUsersWallpaper(url);
    }

    function action_next() {
        imageWallpaper.nextSlide();
    }

    function action_open() {
        Qt.openUrlExternally(currentImage.source)
    }

    //private
    function setupImage() {
        currentImage.sourceSize = root.sourceSize;
        currentImage.fillMode = root.fillMode;
        currentImage.source = modelImage;
    }

    function fadeWallpaper() {
        if (startupTimer.running) {
            setupImage();
            return;
        }

        fadeAnim.running = false
        swapImages()
        currentImage.source = modelImage
        currentImage.sourceSize = root.sourceSize
        // Prevent source size change when image has just been setup anyway
        sourceSizeTimer.stop()
        currentImage.opacity = 0
        currentBlurBackground.opacity = 0
        otherImage.z = 0
        currentImage.z = 1

        // only cross-fade if the new image could be smaller than the old one
        fadeOtherAnimator.enabled = Qt.binding(function() {
            return currentImage.paintedWidth < otherImage.paintedWidth || currentImage.paintedHeight < otherImage.paintedHeight
        })

        // Alleviate stuttering by waiting with the fade animation until the image is loaded (or failed to)
        fadeAnim.running = Qt.binding(function() {
            return currentImage.status !== Image.Loading && otherImage.status !== Image.Loading
        })
    }

    function fadeFillMode() {
        if (startupTimer.running) {
            setupImage();
            return;
        }

        fadeAnim.running = false
        swapImages()
        currentImage.sourceSize = root.sourceSize
        sourceSizeTimer.stop()
        currentImage.source = modelImage
        currentImage.opacity = 0
        currentBlurBackground.opacity = 0
        otherImage.z = 0
        currentImage.fillMode = fillMode
        currentImage.z = 1

        // only cross-fade if the new image could be smaller than the old one
        fadeOtherAnimator.enabled = Qt.binding(function() {
            return currentImage.paintedWidth < otherImage.paintedWidth || currentImage.paintedHeight < otherImage.paintedHeight
        })

        fadeAnim.running = Qt.binding(function() {
            return currentImage.status !== Image.Loading && otherImage.status !== Image.Loading
        })
    }

    function fadeSourceSize() {
        if (currentImage.sourceSize === root.sourceSize) {
            return
        }

        if (startupTimer.running) {
            setupImage();
            return;
        }

        fadeAnim.running = false
        swapImages()
        currentImage.sourceSize = root.sourceSize
        currentImage.opacity = 0
        currentBlurBackground.opacity = 0
        currentImage.source = otherImage.source
        otherImage.z = 0
        currentImage.z = 1

        fadeOtherAnimator.enabled = false // the image size didn't change, avoid cross-dissolve
        fadeAnim.running = Qt.binding(function() {
            return currentImage.status !== Image.Loading && otherImage.status !== Image.Loading
        })
    }

    function startFadeSourceTimer() {
        if (width > 0 && height > 0 && (imageA.status !== Image.Null || imageB.status !== Image.Null)) {
            sourceSizeTimer.restart()
        }
    }

    function swapImages() {
        if (currentImage == imageA) {
            currentImage = imageB
            currentBlurBackground = blurBackgroundB
            otherImage = imageA
            otherBlurBackground = blurBackgroundA
        } else {
            currentImage = imageA
            currentBlurBackground = blurBackgroundA
            otherImage = imageB
            otherBlurBackground = blurBackgroundB
        }
    }

    onWidthChanged: startFadeSourceTimer()
    onHeightChanged: startFadeSourceTimer()

    // HACK prevent fades and transitions during startup
    Timer {
        id: startupTimer
        interval: 100
        running: true
    }

    Timer {
        id: sourceSizeTimer
        interval: 1000 // always delay reloading the image even when animations are turned off
        onTriggered: fadeSourceSize()
    }

    Component.onCompleted: {
        if (wallpaper.pluginName == "org.kde.slideshow") {
            wallpaper.setAction("open", i18nd("plasma_applet_org.kde.image", "Open Wallpaper Image"), "document-open");
            wallpaper.setAction("next", i18nd("plasma_applet_org.kde.image","Next Wallpaper Image"),"user-desktop");
        }
    }

    Wallpaper.Image {
        id: imageWallpaper
        //the oneliner of difference between image and slideshow wallpapers
        renderingMode: (wallpaper.pluginName == "org.kde.image") ? Wallpaper.Image.SingleImage : Wallpaper.Image.SlideShow
        targetSize: Qt.size(root.width, root.height)
        slidePaths: wallpaper.configuration.SlidePaths
        slideTimer: wallpaper.configuration.SlideInterval
    }

    onFillModeChanged: {
        fadeFillMode();
    }
    onConfiguredImageChanged: {
        imageWallpaper.addUrl(configuredImage)
    }
    onModelImageChanged: {
        fadeWallpaper();
    }

    SequentialAnimation {
        id: fadeAnim
        running: false

        ParallelAnimation {
            OpacityAnimator {
                target: currentBlurBackground
                from: 0
                to: 1
                duration: fadeOtherAnimator.duration
            }
            OpacityAnimator {
                target: otherBlurBackground
                from: 1
                // cannot disable an animation individually, so we just fade from 1 to 1
                to: enabled ? 0 : 1

                //use configured duration if animations are enabled
                duration: units.longDuration && wallpaper.configuration.TransitionAnimationDuration
            }
            OpacityAnimator {
                target: currentImage
                from: 0
                to: 1
                duration: fadeOtherAnimator.duration
            }
            OpacityAnimator {
                id: fadeOtherAnimator
                property bool enabled: true
                target: otherImage
                from: 1
                // cannot disable an animation individually, so we just fade from 1 to 1
                to: enabled ? 0 : 1

                //use configured duration if animations are enabled
                duration: units.longDuration && wallpaper.configuration.TransitionAnimationDuration
            }
        }
        ScriptAction {
            script: {
                otherImage.source = "";
                otherImage.fillMode = fillMode;
            }
        }
    }

    Rectangle {
        id: backgroundColor
        anchors.fill: parent
        visible: currentImage.status === Image.Ready || otherImage.status === Image.Ready
        color: wallpaper.configuration.Color
        Behavior on color {
            ColorAnimation { duration: units.longDuration }
            enabled: !startupTimer.running
        }
    }

    Image {
        id: blurBackgroundSourceA
        visible: wallpaper.configuration.Blur
        anchors.fill: parent
        asynchronous: true
        cache: false
        fillMode: Image.PreserveAspectCrop
        source: imageA.source
        z: -1
    }

    GaussianBlur {
        id: blurBackgroundA
        visible: wallpaper.configuration.Blur
        anchors.fill: parent
        source: blurBackgroundSourceA
        radius: 32
        samples: 65
        z: imageA.z
    }

    Image {
        id: blurBackgroundSourceB
        visible: wallpaper.configuration.Blur
        anchors.fill: parent
        asynchronous: true
        cache: false
        fillMode: Image.PreserveAspectCrop
        source: imageB.source
        z: -1
    }

    GaussianBlur {
        id: blurBackgroundB
        visible: wallpaper.configuration.Blur
        anchors.fill: parent
        source: blurBackgroundSourceB
        radius: 32
        samples: 65
        z: imageB.z
    }

    Image {
        id: imageA
        anchors.fill: parent
        asynchronous: true
        cache: false
        fillMode: wallpaper.configuration.FillMode
        autoTransform: true //new API in Qt 5.5, do not backport into Plasma 5.4.
    }

    Image {
        id: imageB
        anchors.fill: parent
        asynchronous: true
        cache: false
        fillMode: wallpaper.configuration.FillMode
        autoTransform: true //new API in Qt 5.5, do not backport into Plasma 5.4.
    }
}
