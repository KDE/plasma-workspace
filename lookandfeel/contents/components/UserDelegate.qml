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

import QtQuick 2.2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: wrapper

    property bool isCurrent: ListView.isCurrentItem

    property string name
    property string userName
    property string iconSource
    property int faceSize: frame.width

    signal clicked()

    height: faceSize + loginText.implicitHeight

    opacity: isCurrent ? 1.0 : 0.618

    Behavior on opacity {
        NumberAnimation { duration: 250 }
    }

    Item {
        id: imageWrapper
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        height: parent.height - loginText.height

        //TODO there code was to show a blue border on mouseover
        //which shows that something is interactable.
        //we can't have that whilst using widgets/background as the base
        //I'd quite like it back

        PlasmaCore.FrameSvgItem {
            id: frame
            imagePath: "widgets/background"

            //width is set in alias at top
            width: Math.round(faceSize * (isCurrent ? 1.0 : 0.8))
            height: width

            Behavior on width {
                NumberAnimation {
                    duration: 100
                }
            }
            anchors {
                centerIn: parent
            }
        }

        //we sometimes have a path to an image sometimes an icon
        //IconItem in it's infinite wisdom tries to load a full path as an icon which is rubbish
        //we try loading it as a normal image, if that fails we fall back to IconItem
        Image {
            id: face
            source: wrapper.iconSource
            anchors {
                fill: frame
                //negative to make frame around the image
                topMargin: frame.margins.top
                leftMargin: frame.margins.left
                rightMargin: frame.margins.right
                bottomMargin: frame.margins.bottom
            }
        }

        PlasmaCore.IconItem {
            id: faceIcon
            source: wrapper.iconSource
            visible: face.status == Image.Error
            anchors.fill: face
        }
    }

    BreezeLabel {
        id: loginText
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        text: wrapper.name
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        maximumLineCount: 2
        wrapMode: Text.Wrap
        //make an indication that this has active focus, this only happens when reached with text navigation
        font.underline: wrapper.activeFocus
        height: Math.round(Math.max(paintedHeight, theme.mSize(theme.defaultFont).height*1.2))
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onClicked: wrapper.clicked();
    }

    Accessible.name: name
    Accessible.role: Accessible.Button
    function accessiblePressAction() { wrapper.clicked() }
}
