/*
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.4

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: wrapper

    property bool isCurrent: ListView.isCurrentItem

    property string name
    property string userName
    property string iconSource

    implicitWidth: units.gridUnit * 10 //6 wide + 1 either side spacing
    implicitHeight: units.gridUnit * 9  + (userName ? usernameDelegate.implicitHeight : 0)

    signal clicked()

    Item {
        id: imageSource
        implicitWidth: units.gridUnit * 8
        implicitHeight: implicitWidth

        //we sometimes have a path to an image sometimes an icon
        //IconItem tries to load a full path as an icon which is rubbish
        //we try loading it as a normal image, if that fails we fall back to IconItem
        Image {
            id: face
            source: wrapper.iconSource
            fillMode: Image.PreserveAspectCrop
            anchors.fill: parent
        }

        PlasmaCore.IconItem {
            id: faceIcon
            source: (face.status === Image.Error || face.status === Image.Null) ?
                    (userName ? "user-identity" : "user-none") : undefined
            visible: valid
            anchors.fill: parent
        }
    }

    ShaderEffect {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter

        implicitWidth: imageSource.width
        implicitHeight: imageSource.height

        supportsAtlasTextures: true

        property var source: ShaderEffectSource {
            sourceItem: imageSource
            hideSource: true
            live: false
        }

        property var colorBorder: theme.buttonFocusColor

        //draw a circle with an antialised border
        //innerRadius = size of the inner circle with contents
        //outerRadius = size of the border
        //blend = area to blend between two colours
        //all sizes are normalised so 0.5 == half the width of the texture

        //if copying into another project don't forget to * qt_Opacity. It's just unused here
        //and connect themeChanged to update()
        fragmentShader: "
                        varying highp vec2 qt_TexCoord0;
                        uniform highp float qt_Opacity;
                        uniform lowp sampler2D source;

                        uniform vec4 colorBorder;
                        float blend = 0.01;
                        float innerRadius = 0.48;
                        float outerRadius = innerRadius + 0.02;
                        vec4 colorEmpty = vec4(0.0, 0.0, 0.0, 0.0);

                        void main() {
                            vec4 colorSource = texture2D(source, qt_TexCoord0.st);

                            vec2 m = qt_TexCoord0 - vec2(0.5, 0.5);
                            float dist = sqrt(m.x * m.x + m.y * m.y);

                            if (dist < innerRadius)
                                gl_FragColor = colorSource;
                            else if (dist < innerRadius + blend)
                                gl_FragColor = mix(colorSource, colorBorder, ((dist - innerRadius) / blend));
                            else if (dist < outerRadius)
                                gl_FragColor = colorBorder;
                            else if (dist < outerRadius + blend)
                                gl_FragColor = mix(colorBorder, colorEmpty, ((dist - outerRadius) / blend));
                            else
                                gl_FragColor = colorEmpty;
                    }
        "
    }

    PlasmaComponents.Label {
        id: usernameDelegate
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        text: wrapper.name
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        //make an indication that this has active focus, this only happens when reached with text navigation
        font.underline: wrapper.activeFocus
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onClicked: wrapper.clicked();
    }

    Accessible.name: name
    Accessible.role: Accessible.Button
    Accessible.onPressAction: wrapper.clicked()
}
