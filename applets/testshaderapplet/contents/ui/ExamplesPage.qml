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

import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.qtextracomponents 0.1 as QtExtras

// ButtonsPage

PlasmaComponents.Page {
    id: examplesPage

    //property string shader
    property string pageName: "Shader Examples"
    property string icon: "weather-clear"

    anchors {
        fill: parent
        margins: _s
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
//     SimpleExample {
//         id: simpleShader
//         //parent: root
//         //source: effectSource
//     }
    Image {
        id: imageItem
        width: 200
        height: 160
        anchors.centerIn: parent
        ///source: "http://vizzzion.org/blog/wp-content/uploads/2013/01/ktouch.png"
        source: "file:///home/sebas/Pictures/Damselfly.jpg"
    }
//     TubeExample {
//         id: tubeShader
//         anchors.fill: imageItem
//         source: ShaderEffectSource {
//             //sourceRect: Qt.rect(-50, -50, width+100, height+100)
//             //sourceRect: Qt.rect(10, 20, 50, 80)
//             sourceItem: imageItem
//             hideSource: hideSourceCheckbox.checked
//         }
//     }
//     GridView {
//         id: grid
//         anchors {
//             top: heading.bottom;
//             topMargin: _s
//             left: parent.left
//             right: parent.right
//             bottom: applyButton.top
//             bottomMargin: _s
//
//         }
//         model: VisualItemModel {
//             PlasmaComponents.Button {
//                 checkable: true;
//                 text: "Simple";
//                 onClicked: {
//                     simpleShader.visible = checked;
// //                     fragmentPage.shaderText = simpleShader.fragmentShader;
// //                     vertexPage.shaderText = simpleShader.vertexShader;
//                 }
//             }
//             PlasmaComponents.Button {
//                 checkable: true;
//                 text: "Tube";
//                 onClicked: {
//                     tubeShader.visible = checked;
//                     mainItem.opacity = checked ? 0.1 : 0.1
// //                     fragmentPage.shaderText = tubeShader.fragmentShader;
// //                     vertexPage.shaderText = tubeShader.vertexShader;
//                 }
//             }
//         }
//     }
}

