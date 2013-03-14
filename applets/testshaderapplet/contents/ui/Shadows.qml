/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
//import QtQuick.Particles 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

ShaderExample {

    pageName: "Shadows"
    pageDescription: ""
    Grid {
        id: cfgrid
        columns: 2

        anchors.top: parent.top
        anchors.right: parent.right
        width: parent.width * 0.6
        height: 96
        spacing: 6
        columnSpacing: 12
        PlasmaComponents.Label {
            text: "Distance:";
            width: parent.width * 0.5;
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
        }
        PlasmaComponents.Slider {
            width: parent.width * 0.4
            id: distanceSlider
            stepSize: 1
            minimumValue: 0
            maximumValue: 25
            value: 8
        }

        PlasmaComponents.Label {
            text: "Opacity:";
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            width: parent.width * 0.5;
        }
        PlasmaComponents.Slider {
            width: parent.width * 0.4
            id: opacitySlider
//             stepSize: 250
            minimumValue: 0
            maximumValue: 1.0
            stepSize: 0.05
            value: 0.4
        }
    }

    Item {
        anchors.fill: parent
        clip: true
        anchors.margins: -_s

        Item {
            id: theItem
            anchors.fill: parent
            anchors.topMargin: _s * 2

            Column {
                id: "txtCol"
                anchors.fill: parent
                anchors.margins: _s
                anchors.topMargin: _s * 2
                spacing: _s

                PlasmaExtras.Heading {
                    text: "Effects on Components"

                }
                PlasmaExtras.Paragraph {
                    text:"In Plasma 2, the user interface is based on an OpenGL scenegraph, composition of the UI happens on the graphics card. In the past weeks, we've been working on getting a Plasma 2 shell up and running, and on porting the QML imports of the Plasma Components."
                }

                PlasmaExtras.Heading {
                    font.pointSize: 48
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.topMargin: _s * 3
                    text: "This is shadowy text."
                }
                Row {
                    height: 48
                    width: parent.width
                    spacing: _s

                    PlasmaCore.IconItem {
                        source: "configure"
                        width: parent.height
                        height: width
                    }
                    PlasmaCore.IconItem {
                        source: "dialog-ok"
                        width: parent.height
                        height: width
                    }

                    PlasmaCore.SvgItem {
                        id: buttonItem
                        svg: PlasmaCore.Svg {
                            id: configIconsSvg
                            imagePath: "widgets/configuration-icons"
                        }
                        elementId: "maximize"
                        width: parent.height
                        height: width
                    }

                    PlasmaCore.IconItem {
                        source: "akonadi"
                        width: parent.height
                        height: width
                    }
                    PlasmaCore.IconItem {
                        source: "clock"
                        width: parent.height
                        height: width
                    }
                }
            }
        }
        ShaderEffectSource {
            id: theSource
            sourceItem: theItem
            hideSource: true
        }

        ShaderEffect {
            anchors.fill: theItem
            property variant source: theSource
            property variant shadow: ShaderEffectSource {
                sourceItem: ShaderEffect {
                    width: theItem.width
                    height: theItem.height
                    property variant delta: Qt.size(0.0, 1.0 / height)
                    property variant source: ShaderEffectSource {
                        sourceItem: ShaderEffect {
                            width: theItem.width
                            height: theItem.height
                            property variant delta: Qt.size(1.0 / width, 0.0)
                            property variant source: theSource
                            fragmentShader: "
                                uniform lowp float qt_Opacity;
                                uniform sampler2D source;
                                uniform highp vec2 delta;
                                varying highp vec2 qt_TexCoord0;
                                void main() {
                                    gl_FragColor =(0.0538 * texture2D(source, qt_TexCoord0 - 3.182 * delta)
                                                 + 0.3229 * texture2D(source, qt_TexCoord0 - 1.364 * delta)
                                                 + 0.2466 * texture2D(source, qt_TexCoord0)
                                                 + 0.3229 * texture2D(source, qt_TexCoord0 + 1.364 * delta)
                                                 + 0.0538 * texture2D(source, qt_TexCoord0 + 3.182 * delta)) * qt_Opacity;
                                }"
                        }
                    }
                    fragmentShader: "
                        uniform lowp float qt_Opacity;
                        uniform sampler2D source;
                        uniform highp vec2 delta;
                        varying highp vec2 qt_TexCoord0;
                        void main() {
                            gl_FragColor =(0.0538 * texture2D(source, qt_TexCoord0 - 3.182 * delta)
                                         + 0.3229 * texture2D(source, qt_TexCoord0 - 1.364 * delta)
                                         + 0.2466 * texture2D(source, qt_TexCoord0)
                                         + 0.3229 * texture2D(source, qt_TexCoord0 + 1.364 * delta)
                                         + 0.0538 * texture2D(source, qt_TexCoord0 + 3.182 * delta)) * qt_Opacity;
                        }"
                }
            }
            property real angle: 0
            property variant offset: Qt.point(distanceSlider.value * Math.cos(angle), distanceSlider.value * Math.sin(angle))
            NumberAnimation on angle { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 6000 }
            property variant delta: Qt.size(offset.x / width, offset.y / height)
            property real darkness: opacitySlider.value // Changeme
            fragmentShader: "
                uniform lowp float qt_Opacity;
                uniform highp vec2 offset;
                uniform sampler2D source;
                uniform sampler2D shadow;
                uniform highp float darkness;
                uniform highp vec2 delta;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    lowp vec4 fg = texture2D(source, qt_TexCoord0);
                    lowp vec4 bg = texture2D(shadow, qt_TexCoord0 + delta);
                    gl_FragColor = (fg + vec4(0., 0., 0., darkness * bg.a) * (1. - fg.a)) * qt_Opacity;
                }"
//             Slider {
//                 id: shadowSlider
//                 anchors.left: parent.left
//                 anchors.right: parent.right
//                 anchors.bottom: parent.bottom
//                 height: 40
//             }
        }

    }
}