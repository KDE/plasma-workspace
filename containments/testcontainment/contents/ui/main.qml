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
        PlasmaCore.FrameSvgItem {
            id: frame
            x: 50
            y: 50

            width: large + frame.margins.left + frame.margins.right
            height: large + frame.margins.top + frame.margins.bottom

            property alias applet: appletContainer.children
            onAppletChanged: {
                if (appletContainer.children.length == 0) {
                    killAnim.running = true
                }
            }

            property int small: 90
            property int large: 400

            property int tm: 0
            property int lm: 0

            imagePath: applet.length > 0 && applet[0].backgroundHints == 0 ? "" : "widgets/background"

            onImagePathChanged: {
                // Reposition applet so it fits into the frame
                if (imagePath == "") {
                    frame.x = frame.x + lm;
                    frame.y = frame.y + tm;
                } else {
                    // Cache values, so we can subtract them when the background is removed
                    frame.lm = frame.margins.left;
                    frame.tm = frame.margins.top;

                    frame.x = frame.x - frame.margins.left;
                    frame.y = frame.y - frame.margins.top;
                }
            }
            MouseArea {
                anchors.fill: parent
                drag.target: parent
                onClicked: {
                    var s = (frame.width == frame.large) ? frame.small : frame.large;
                    frame.height = s
                    frame.width = s
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
    }

    Component.onCompleted: {
        print("Test Containment loaded")
        print(plasmoid)
    }
}