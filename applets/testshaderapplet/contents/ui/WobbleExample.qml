/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
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

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

ShaderExample {

    pageName: "Wobble"
    pageDescription: "Makes an image wobble"

    Grid {
        id: cfgrid
        columns: 2

        anchors.top: parent.top
        anchors.right: parent.right
        width: parent.width * 0.6
        height: 96
        spacing: 6
        columnSpacing: _m
        PlasmaComponents.Label {
            text: "Amplitude:";
            width: parent.width * 0.5;
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
        }
        PlasmaComponents.Slider {
            width: parent.width * 0.4
            id: amplitudeSlider
            stepSize: 0.05
            minimumValue: 0
            maximumValue: 1.0
            value: 0.4
        }

        PlasmaComponents.Label {
            text: "Speed:";
            horizontalAlignment: Text.AlignRight
            width: parent.width * 0.5;
            elide: Text.ElideRight
        }
        PlasmaComponents.Slider {
            width: parent.width * 0.4
            id: speedSlider
            stepSize: 250
            minimumValue: 0
            maximumValue: 6000
            value: 3000
            onValueChanged: {
                if (timeAnimation != null) {
                    timeAnimation.duration = maximumValue - value +1;
                    timeAnimation.restart();
                }
            }
        }
    }
    PlasmaComponents.Button {
        anchors { right: parent.right; bottom: parent.bottom; }
//         height: theme.iconSizes.toolbar
        text: "Busy"
        checked: plasmoid.busy
        onClicked: {
            plasmoid.busy = !plasmoid.busy
        }
    }


    Item {
        id: imageItem
        opacity: 0.8
        anchors.fill: parent
        anchors.topMargin: 48
        Image {
            source: "../images/elarun-small.png"
            anchors.fill: parent
            anchors.margins: parent.height / 10
        }
    }

//     PlasmaCore.IconItem {
//         id: iconItem
//         source: "plasmagik"
//         width: 400
//         height: 400
// //         width: parent.height
// //         height: width
//         anchors.centerIn: parent
//     }

    ShaderEffect {
        id: wobbleShader

        anchors.fill: imageItem
        //property real time
        property variant mouse
        property variant resolution

        property int fadeDuration: 250
        property real amplitude: 0.04 * amplitudeSlider.value
        property real frequency: 20
        property real time: 10
        property int speed: (speedSlider.maximumValue - speedSlider.value + 1)

        property variant source: ShaderEffectSource {
            sourceItem: imageItem
            hideSource: true
        }

        NumberAnimation on time { id: timeAnimation; loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 3000 }
        Behavior on amplitude { NumberAnimation { duration: wobbleShader.fadeDuration } }

        fragmentShader: { //mainItem.opacity = 0;
            "uniform lowp float qt_Opacity;" +
            "uniform highp float amplitude;" +
            "uniform highp float frequency;" +
            "uniform highp float time;" +
            "uniform sampler2D source;" +
            "varying highp vec2 qt_TexCoord0;" +
            "void main() {" +
            "    highp vec2 p = sin(time + frequency * qt_TexCoord0);" +
            "    gl_FragColor = texture2D(source, qt_TexCoord0 + amplitude * vec2(p.y, -p.x)) * qt_Opacity;" +
            "}"
        }
    }

}
