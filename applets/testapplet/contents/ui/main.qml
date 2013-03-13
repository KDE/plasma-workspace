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

Rectangle {
    id: root
    color: "transparent"
    width: 100
    height: 100
    radius: 10
    smooth: true

    property Component compactRepresentation: Component {
        Rectangle {
            MouseArea {
                anchors.fill: parent
                onClicked: plasmoid.expanded = !plasmoid.expanded
            }
        }
    }

    Column {
        anchors.centerIn: parent
        Text {
            text: "I'm an applet"
        }
        PlasmaComponents.Button {  
            text: "Background"
            checked: plasmoid.backgroundHints == 1
            onClicked: {
                print("Background hints: " + plasmoid.backgroundHints)
                if (plasmoid.backgroundHints == 0) {
                    plasmoid.backgroundHints = 1//TODO: make work "StandardBackground"
                    root.color = "transparent"
                } else {
                    plasmoid.backgroundHints = 0//TODO: make work "NoBackground"
                    root.color = "darkgreen"
                }
            }
        }
        PlasmaComponents.Button {  
            text: "Busy"
            checked: plasmoid.busy
            onClicked: {
                plasmoid.busy = !plasmoid.busy
            }
        }
        TextInput {
            width: 100
            height: 22
            text: plasmoid.configuration.Test
            onTextChanged: plasmoid.configuration.Test = text
        }
    }
    Component.onCompleted: {
        print("Test Applet loaded")
    }
}