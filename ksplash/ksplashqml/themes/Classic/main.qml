/*
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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

