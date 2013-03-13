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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

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
                    id: mouseArea
                    
                    property real dx: 0
                    property real dy: 0
                    property real startX
                    property real startY
                    
                    anchors.fill: parent
                    drag.target: frameParent
                    onClicked: {
                        var s = (frameParent.width == frameParent.large) ? frameParent.small : frameParent.large;
                        frameParent.height = s
                        frameParent.width = s
                    }
                    onPressed: {
                        speedSampleTimer.running = true
                        mouseArea.startX = mouse.x
                        mouseArea.startY = mouse.y
                        speedSampleTimer.lastFrameParentX = frameParent.x
                        speedSampleTimer.lastFrameParentY = frameParent.y
                    }
                    onPositionChanged: {
                        //mouseArea.dx = mouse.x - mouseArea.startX
                        //mouseArea.dy = mouse.y - mouseArea.startY
                        dxAnim.running = false
                        dyAnim.running = false
                    }
                    onReleased: {
                        speedSampleTimer.running = false
                        dxAnim.running = true
                        dyAnim.running = true
                    }
                    Timer {
                        id: speedSampleTimer
                        interval: 40
                        repeat: true
                        property real lastFrameParentX
                        property real lastFrameParentY
                        onTriggered: {
                            mouseArea.dx = frameParent.x - lastFrameParentX
                            mouseArea.dy = frameParent.y - lastFrameParentY
                            lastFrameParentX = frameParent.x
                            lastFrameParentY = frameParent.y
                            dxAnim.running = true
                            dyAnim.running = true
                        }
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
                    hideSource: true
                    sourceItem: frame
                }

                property int fadeDuration: 150
                property real time: 0
                property real dx: mouseArea.dx
                property real dy: mouseArea.dy
                property real startX: mouseArea.startX/mouseArea.width
                property real startY: mouseArea.startY/mouseArea.height
                
                NumberAnimation on dx { id: dxAnim; to: 0; duration: 350; easing.type: Easing.OutElastic }
                NumberAnimation on dy { id: dyAnim; to: 0; duration: 350; easing.type: Easing.OutElastic }
                //! [fragment]
                fragmentShader: {
                    "uniform lowp float qt_Opacity;" +
                    "uniform highp float dx;" +
                    "uniform highp float dy;" +
                    "uniform highp float startX;" +
                    "uniform highp float startY;" +
                    "uniform sampler2D source;" +
                    "varying highp vec2 qt_TexCoord0;" +
                    "void main() {" +

                    "highp float wave_x = qt_TexCoord0.x - dx*0.0025" +
                    "                  * (1.0/(1.0+abs(qt_TexCoord0.x-(startX))))" +
                    "                  * (1.0/(1.0+abs(qt_TexCoord0.y-(startY))));" +
                    "highp float wave_y = qt_TexCoord0.y - dy*0.0025" +
                    "                  * 1.0/(1.0+abs(qt_TexCoord0.x-(startX)))" +
                    "                  * 1.0/(1.0+abs(qt_TexCoord0.y-(startY)));" +
                    "gl_FragColor = texture2D(source, vec2(wave_x, wave_y));" +
                    "}"
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