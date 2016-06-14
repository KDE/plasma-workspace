/*
 *   Copyright 2014 Marco Martin <mart@kde.org>
 *   Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>
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

import QtQuick 2.5

Item {
    property int stage

    onStageChanged: {
        if (stage == 1) {
            introAnimation.start()
        }
    }

    TextMetrics {
        id: units
        text: "M"
        readonly property int gridUnit: boundingRect.height
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#1fb4f9"
            }
            GradientStop {
                position: 1.0
                color: "#197cf1"
            }
        }
    }

    Column {
        id: content
        anchors.centerIn: parent
        spacing: units.gridUnit * 3
        opacity: 0

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "images/kde.svgz"
            sourceSize.height: units.gridUnit * 8
            sourceSize.width: units.gridUnit * 8
        }

        Rectangle {
            radius: height
            color: "#31363b"
            height: Math.round(units.gridUnit / 2)
            width: height * 32

            Rectangle {
                radius: height
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                }
                width: Math.round((parent.width / 5) * (stage - 1))
                Behavior on width {
                    PropertyAnimation {
                        duration: 250
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }
    }

    OpacityAnimator {
        id: introAnimation
        target: content
        from: 0
        to: 1
        duration: 1000
        easing.type: Easing.InOutQuad
    }
}
