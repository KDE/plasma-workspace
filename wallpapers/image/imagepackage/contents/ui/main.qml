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

import QtQuick 2.2
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    id: root

    readonly property string configuredImage: wallpaper.configuration.Image
    readonly property string modelImage: imageWallpaper.wallpaperPath
    property Item currentImage: imageB
    property Item otherImage: imageA
    readonly property int fillMode: wallpaper.configuration.FillMode

    //public API, the C++ part will look for those
    function setUrl(url) {
        wallpaper.configuration.Image = url
        imageWallpaper.addUsersWallpaper(url);
    }

    function action_next() {
        imageWallpaper.nextSlide();
    }

    //private
    function fadeWallpaper() {
        fadeAnim.running = false
        if (currentImage == imageA) {
            currentImage = imageB
            otherImage = imageA
        } else {
            currentImage = imageA
            otherImage = imageB
        }
        currentImage.source = modelImage
        currentImage.opacity = 0
        otherImage.z = 0
        currentImage.z = 1
        // Alleviate stuttering by waiting with the fade animation until the image is loaded (or failed to)
        fadeAnim.running = Qt.binding(function() {
            return currentImage.status !== Image.Loading && otherImage.status !== Image.Loading
        })
    }

    function fadeFillMode() {
        fadeAnim.running = false
        if (currentImage == imageA) {
            currentImage = imageB
            otherImage = imageA
        } else {
            currentImage = imageA
            otherImage = imageB
        }
        currentImage.source = modelImage
        currentImage.opacity = 0
        otherImage.z = 0
        currentImage.fillMode = fillMode
        currentImage.z = 1
        fadeAnim.running = true
    }

    Binding {
        target: wallpaper.configuration
        property: "width"
        value: root.width
    }
    Binding {
        target: wallpaper.configuration
        property: "height"
        value: root.height
    }

    Component.onCompleted: {
        if (wallpaper.pluginName == "org.kde.slideshow") {
            wallpaper.setAction("next", i18nd("plasma_applet_org.kde.image","Next Wallpaper Image"),"user-desktop");
        }

        if (!configuredImage && imageWallpaper.wallpaperPath) {
            return;
        }
        imageWallpaper.addUrl(configuredImage)
        fadeWallpaper()
    }

    Wallpaper.Image {
        id: imageWallpaper
        //the oneliner of difference between image and slideshow wallpapers
        renderingMode: (wallpaper.pluginName == "org.kde.image") ? Wallpaper.Image.SingleImage : Wallpaper.Image.SlideShow
//         targetSize: "1920x1080"
        width: root.width
        height: root.height
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
        OpacityAnimator {
            target: currentImage
            from: 0
            to: 1
            duration: units.longDuration
        }
        ScriptAction {
            script: {
                otherImage.opacity = 0;
                otherImage.fillMode = fillMode;
                otherImage.source = "";
            }
        }
    }

    Rectangle {
        id: backgroundColor
        anchors.fill: parent

        visible: currentImage.fillMode === Image.PreserveAspectFit || currentImage.fillMode === Image.Pad
              || otherImage.fillMode === Image.PreserveAspectFit || otherImage.fillMode === Image.Pad
        color: wallpaper.configuration.Color
        Behavior on color {
            ColorAnimation { duration: units.longDuration }
        }
    }

    Image {
        id: imageA
        anchors.fill: parent
        asynchronous: true
        cache: false
        fillMode: wallpaper.configuration.FillMode
        sourceSize: Qt.size(width, height)
    }
    Image {
        id: imageB
        anchors.fill: parent
        asynchronous: true
        cache: false
        fillMode: wallpaper.configuration.FillMode
        sourceSize: Qt.size(width, height)
    }
}
