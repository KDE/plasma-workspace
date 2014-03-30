/*
 *   Copyright (C) 2013 Martin Klapetek <mklapetek(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0

Item {
    id: main

    width: screenSize.width
    height: screenSize.height

    property int stage
    property int iconSize: 78

    onStageChanged: {
        if (stage == 1) {
            iconsBackgroundRect.opacity = 1
        }
        if (stage == 2) {
            icon1.opacity = 1
        }
        if (stage == 3) {
            icon2.opacity = 1
        }
        if (stage == 4) {
            icon3.opacity = 1
        }
        if (stage == 5) {
            icon4.opacity = 1
        }
        if (stage == 6) {
            icon5.opacity = 1
        }
    }

    Image {
        id: background
        anchors.fill: parent
        source: "images/background.png"
    }

    Image {
        id: iconsBackgroundRect
        anchors.centerIn: main
        width: row.implicitWidth + 32 // 32 being margins
        source: "images/rectangle.png"

        opacity: 0
        Behavior on opacity { FadeIn {} }

        Row {
            id: row
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.height

            Image {
                id: icon1
                anchors.verticalCenter: parent.verticalCenter
                width: iconSize
                source: "images/icon1.png"

                opacity: 0
                Behavior on opacity { FadeIn {} }
            }

            Image {
                id: icon2
                anchors.verticalCenter: parent.verticalCenter
                width: iconSize
                source: "images/icon2.png"

                opacity: 0
                Behavior on opacity { FadeIn {} }
            }


            Image {
                id: icon3
                anchors.verticalCenter: parent.verticalCenter
                width: iconSize
                source: "images/icon3.png"

                opacity: 0
                Behavior on opacity { FadeIn {} }
            }


            Image {
                id: icon4
                anchors.verticalCenter: parent.verticalCenter
                width: iconSize
                source: "images/icon4.png"

                opacity: 0
                Behavior on opacity { FadeIn {} }
            }


            Image {
                id: icon5
                anchors.verticalCenter: parent.verticalCenter
                width: 125
                source: "images/icon5.png"

                opacity: 0
                Behavior on opacity { FadeIn {} }
            }
        }
    }
}

