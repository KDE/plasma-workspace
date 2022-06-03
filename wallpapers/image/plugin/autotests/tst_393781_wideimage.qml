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

    width: 300 // 100 + 200
    height: 100

    when: windowShown
    name: "WideImageProviderTest"

    Component {
        id: mainImage

        Image {
            anchors.fill: parent
            asynchronous: false
            fillMode: Image.Stretch

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
        title: testWideImage.toString()
    }

    function test_screens_data() {
        return [
            {
                desktopX: 0,
                desktopY: 0,
                desktopWidth: 100,
                desktopHeight: 100,
                color: Qt.rgba(0, 0, 0, 255),
            },
            {
                desktopX: 100,
                desktopY: 0,
                desktopWidth: 200,
                desktopHeight: 100,
                color: Qt.rgba(255, 0, 0, 255),
            },
        ]
    }

    function test_screens(data) {
        verify(testWideImage.toString().length > 0);

        // 7: remove "file://"
        const path = `image://wideimage/get?path=${testWideImage.toString().slice(7)}&desktopX=${data.desktopX}&desktopY=${data.desktopY}&desktopWidth=${data.desktopWidth}&desktopHeight=${data.desktopHeight}&totalRectX=0&totalRectY=0&totalRectWidth=300&totalRectHeight=100`
        console.debug("Path is", path)
        const image = createTemporaryObject(mainImage, window, {
            source: path,
            sourceSize: Qt.size(data.desktopWidth, data.desktopHeight)
        });

        image.wait();
        compare(image.status, Image.Ready);

        const grabbed = grabImage(image);
        compare(grabbed.pixel(0, 0), data.color);
    }
}
