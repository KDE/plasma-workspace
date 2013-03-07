/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
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

import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents

Item {
    id: root
    width: 640
    height: 480

    property Item toolBox

    Connections {
        target: plasmoid
        onAppletAdded: {
            var container = appletContainerComponent.createObject(root)
            container.visible = true
            print("Applet added: " + applet)
            applet.parent = container
            container.applet = applet
            applet.anchors.fill= applet.parent
            applet.visible = true
        }
    }

    Component {
        id: appletContainerComponent
        Item {
            id: frameParent
            x: 50
            y: 50

            property int small: 90
            property int large: 400

            width: large + frame.margins.left + frame.margins.right
            height: large + frame.margins.top + frame.margins.bottom

            property alias applet: appletContainer.children
            onAppletChanged: {
                if (appletContainer.children.length == 0) {
                    killAnim.running = true
                }
            }
            PlasmaCore.FrameSvgItem {
                id: frame
                anchors.fill: parent

                property int tm: 0
                property int lm: 0

                imagePath: applet.length > 0 && applet[0].backgroundHints == 0 ? "" : "widgets/background"

                onImagePathChanged: {
                    // Reposition applet so it fits into the frame
                    if (imagePath == "") {
                        frameParent.x = frameParent.x + lm;
                        frameParent.y = frameParent.y + tm;
                    } else {
                        // Cache values, so we can subtract them when the background is removed
                        frame.lm = frame.margins.left;
                        frame.tm = frame.margins.top;

                        frameParent.x = frameParent.x - frame.margins.left;
                        frameParent.y = frameParent.y - frame.margins.top;
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    drag.target: frameParent
                    onClicked: {
                        var s = (frameParent.width == frameParent.large) ? frameParent.small : frameParent.large;
                        frameParent.height = s
                        frameParent.width = s
                    }
                }

                Item {
                    id: appletContainer
                    anchors {
                        fill: parent
                        leftMargin: frame.margins.left
                        rightMargin: parent.margins.right
                        topMargin: parent.margins.top
                        bottomMargin: parent.margins.bottom
                    }
                }

                PlasmaComponents.BusyIndicator {
                    id: busyIndicator
                    z: 1000
                    visible: applet.length > 0 && applet[0].busy
                    running: visible
                    anchors.centerIn: parent
                }
                SequentialAnimation {
                    id: killAnim
                    NumberAnimation {
                        target: frame
                        properties: "scale"
                        to: 0
                        duration: 250
                    }
                    ScriptAction { script: frame.destroy()}
                }
            }
            ShaderEffect {
                id: wobbleShader
                anchors.fill: frame
                property variant source: ShaderEffectSource {
                    id: theSource
                    sourceItem: frame
                }
                opacity: 0
                property int fadeDuration: 250
                property real amplitude: busyIndicator.visible ? 0.04 * 0.2 : 0
                property real frequency: 20
                property real time: 0
                NumberAnimation on time { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 600 }
                Behavior on amplitude { NumberAnimation { duration: wobbleShader.fadeDuration } }
                //! [fragment]
                fragmentShader: {
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

                // compose the item on-screen, we want to render
                // either the shader item or the source item,
                // so swap their opacity as the wobbling fades in
                // and after it fades out
                Connections {
                    target: busyIndicator
                    onVisibleChanged: {
                        if (busyIndicator.visible) {
                            wobbleShader.opacity = 1;
                            frame.opacity = 0;
                        } else {
                             hideTimer.start();
                        }
                    }
                }
                Timer {
                    id: hideTimer
                    interval: wobbleShader.fadeDuration
                    onTriggered: {
                        wobbleShader.opacity = 0;
                        frame.opacity = 1;
                    }
                }
                //! [fragment]
            }
        }
    }

    Component.onCompleted: {
        print("Test Containment loaded")
        print(plasmoid)
    }
}