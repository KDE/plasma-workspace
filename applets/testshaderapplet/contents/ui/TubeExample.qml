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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0

ShaderExample {

    pageName: "Wave"
    pageDescription: "Makes an image wobble"

    Image {
        id: imageItem
        anchors.fill: parent
        anchors.topMargin: 48
    }

    ShaderEffect {

        anchors.fill: imageItem
        //property real time
        property variant mouse
        property variant resolution

        property int fadeDuration: 250
        property real amplitude: 0.04 * 0.4
        property real frequency: 20
        property real time: 0
        source: ShaderEffectSource {
            sourceItem: imageItem
            hideSource: true
        }

        NumberAnimation on time { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 600 }
        Behavior on amplitude { NumberAnimation { duration: fadeDuration } }

        //vertexShader: ""
        fragmentShader: { mainItem.opacity = 0;
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
            /*
        fragmentShader: {
            "uniform float time;  \
            uniform vec2 mouse;  \
            uniform vec2 resolution;  \
            void main( void )  \
            { \
    //             vec2 uPos = ( gl_FragCoord.xy / resolution.xy );//normalize wrt y axis \
    //             //suPos -= vec2((resolution.x/resolution.y)/2.0, 0.0);//shift origin to center \
    //             uPos.x -= 8.0; \
    //             uPos.y -= 0.5; \
    //             vec3 color = vec3(0.0); \
    //             float vertColor = 0.0; \
    //             for( float i = 0.0; i < 15.0; ++i ) { \
    //                 float t = time * (0.9); \
    //                 uPos.y += sin( uPos.x*i + t+i/2.0 ) * 0.1; \
    //                 float fTemp = abs(1.0 / uPos.y / 100.0); \
    //                 vertColor += fTemp; \
    //                 color += vec3( fTemp*(10.0-i)/10.0, fTemp*i/10.0, pow(fTemp,1.5)*1.5 ); \
    //             } \
    //             vec4 color_final = vec4(color, 1.0); \
    //             gl_FragColor = color_final; \
            } \
            "
        */
        }
    }

}
