/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Window 2.15
import QtTest 1.0

import org.kde.plasma.wallpapers.image 2.0 as Wallpaper

TestCase {
    id: root

    width: 320
    height: 240

    when: windowShown
    name: "ImageBackendTest"

    Wallpaper.ImageBackend {
        id: imageWallpaper
        usedInConfig: false
        //the oneliner of difference between image and slideshow wallpapers
        renderingMode: Wallpaper.ImageBackend.SingleImage
        targetSize: Qt.size(root.width, root.height)
    }

    Wallpaper.MediaProxy {
        id: mediaProxy
        source: imageWallpaper.image
        targetSize: imageWallpaper.targetSize
    }

    SignalSpy {
        id: modelImageChangedSignalSpy
        target: mediaProxy
        signalName: "modelImageChanged"
    }

    Component {
        id: mainImage

        Image {
            anchors.fill: parent
            asynchronous: false
            fillMode: Image.Stretch
            sourceSize: imageWallpaper.targetSize

            function wait() {
                while (status === Image.Loading) {
                    statusSpy.wait(5000);
                }
            }

            SignalSpy {
                id: statusSpy
                target: parent
                signalName: "statusChanged"
            }
        }
    }

    Window {
        id: window
        width: root.width
        height: root.height
        visible: true
        title: mediaProxy.modelImage.toString()
    }

    function test_setSingleImage() {
        imageWallpaper.renderingMode = Wallpaper.ImageBackend.SingleImage

        verify(testImage.toString().length > 0);
        imageWallpaper.image = testImage;
        compare(mediaProxy.modelImage, testImage);

        const image = createTemporaryObject(mainImage, window, {source: mediaProxy.modelImage});
        compare(image.status, Image.Ready);
        const grabbed = grabImage(image);
        compare(grabbed.pixel(0, 0), Qt.rgba(0, 0, 0, 255));
    }

    function test_setPackageImage() {
        imageWallpaper.renderingMode = Wallpaper.ImageBackend.SingleImage

        verify(testPackage.toString().length > 0);
        imageWallpaper.image = testPackage;
        compare(mediaProxy.modelImage.toString().indexOf("image://package/get?dir="), 0);

        const image = createTemporaryObject(mainImage, window, {source: mediaProxy.modelImage});
        compare(image.status, Image.Loading);
        image.wait();
        compare(image.status, Image.Ready);
        const grabbed1 = grabImage(image);
        compare(grabbed1.pixel(0, 0), Qt.rgba(255, 255, 255, 255));

        // Change target size
        image.sourceSize = Qt.size(1920, 1080);
        compare(image.status, Image.Loading);
        image.wait();
        compare(image.status, Image.Ready);

        const grabbed2 = grabImage(image);
        compare(grabbed2.pixel(0, 0), Qt.rgba(0, 0, 0, 255));

        // Change backend's target size
        modelImageChangedSignalSpy.clear();
        imageWallpaper.targetSize = Qt.size(root.width + 1, root.height + 1);
        compare(modelImageChangedSignalSpy.count, 1);
    }

    function test_setBizzareFileName() {
        imageWallpaper.renderingMode = Wallpaper.ImageBackend.SingleImage;

        verify(testBizzareFileName.length > 0);
        modelImageChangedSignalSpy.clear();
        mediaProxy.source = testBizzareFileName;
        compare(modelImageChangedSignalSpy.count, 1);
        compare(mediaProxy.providerType, Wallpaper.Provider.Image);
        compare(mediaProxy.modelImage, testBizzareFileName_modelImage);
        console.debug(mediaProxy.modelImage);

        mediaProxy.source = Qt.binding(() => imageWallpaper.image);
    }

    function test_startSlideShow() {
        verify(testDirs[0].length > 0);

        imageWallpaper.slidePaths = testDirs;
        imageWallpaper.slideTimer = 1000; // use nextSlide
        imageWallpaper.slideshowMode = Wallpaper.SortingMode.Alphabetical;
        imageWallpaper.slideshowFoldersFirst = false;

        imageWallpaper.renderingMode = Wallpaper.ImageBackend.SlideShow;
        wait(1000); // &SlideModel::done

        let image = mediaProxy.modelImage;

        imageWallpaper.nextSlide();
        verify(image != mediaProxy.modelImage);

        image = mediaProxy.modelImage;
        imageWallpaper.nextSlide();
        verify(image != mediaProxy.modelImage);
    }
}
