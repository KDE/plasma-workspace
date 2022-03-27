/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQml 2.15

Timer {
    id: generator

    property color randomColor: wallpaper.configuration.Color
    property color previousColor: randomColor

    function nextColor() {
        let count = 0;
        let r, g, b;
        generator.previousColor = randomColor;

        while (count < 10 && generator.previousColor === generator.randomColor) {
            r = Math.random();
            g = Math.random();
            b = Math.random();
            generator.randomColor = Qt.rgba(r, g, b);
            count += 1;
        }
    }

    interval: wallpaper.configuration.SlideInterval * 1000
    repeat: true
    running: true
    triggeredOnStart: true

    onTriggered: generator.nextColor()
}
