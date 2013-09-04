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
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.qtextracomponents 2.0 as QtExtras

// ButtonsPage

PlasmaComponents.Page {
    id: editorPage

    property string shader
    property alias shaderText: editor.text
    property string pageName: "Editor"
    property string icon: "accessories-text-editor"

    anchors {
        fill: parent
        margins: _s
    }

    Image {
        id: imageItem
        anchors.fill: parent
        //source: "../images/elarun-small.png"
    }

    ShaderEffectSource {
        id: effectSource
        sourceItem: imageItem
        //hideSource: hideSourceCheckbox.checked
        hideSource: true
    }

    ShaderEffect {
        id: mainShader
        anchors.fill: editorPage
        property variant source: effectSource
        property real f: 0
        property real f2: 0
        property int intensity: 1
        smooth: true
    }
    PlasmaComponents.ToolButton {
        iconSource: "dialog-close"
        width: _h
        height: width
        visible: !(mainShader.fragmentShader == "" && mainShader.vertexShader == "")
        anchors { top: parent.top; right: parent.right; }
        onClicked: {
            mainShader.fragmentShader = "";
            mainShader.vertexShader = "";
            editorPage.shader = ""
            vertexPage.shader = ""
        }
    }


    PlasmaExtras.Heading {
        id: heading
        level: 1
        anchors {
            top: parent.top;
            left: parent.left
            right: parent.right
        }
        text: pageName
    }
    PlasmaComponents.ButtonColumn {
        anchors {
            right: parent.right
            top: heading.top
        }
        PlasmaComponents.RadioButton {
            id: fragmentRadio
            text: "Fragment / Pixel Shader"
        }
        PlasmaComponents.RadioButton {
            text: "Vertex Shader"
        }
    }

//     PlasmaComponents.TextArea {
//         id: editor
//         anchors {
//             top: heading.bottom;
//             topMargin: _s
//             left: parent.left
//             right: parent.right
//             bottom: applyButton.top
//             bottomMargin: _s
//
//         }
// //         text: { "void main(void) {\
// //         gl_FragColor = vec4(1.0, 0.0, 0.0, 0.3);\
// //     }"
// //         }
//         text:"
//         void main(void) {
//             gl_FragColor = vec4(0.2, 0.8, 0.6, 0.3);
//         }
//         "
//
// //         width: parent.width
// //         parent.height-height: _h*2
//     }

    PlasmaComponents.Button {
        id: applyButton
        text: "Upload Shader"
        onClicked: {
            shader = editor.text
            if (fragmentRadio.checked) {
                print("Uploading new fragment shader: \n" + shader);
                mainShader.fragmentShader = shader
            } else {
                print("Uploading new vertex shader: \n" + shader);
                mainShader.vertexShader = shader;
            }
        }

        anchors {
            right: parent.right
            bottom: parent.bottom

        }



    }
//     PlasmaComponents.CheckBox {
//         id: hideSourceCheckbox
//         text: "Hide Source Item"
//         anchors { bottom: parent.bottom; left: parent.left; margins: _s; }
//         onCheckedChanged: effectSource.hideSource = checked
//     }

}
