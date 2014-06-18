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

import QtQuick 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: wrapper

    property bool isCurrent: ListView.isCurrentItem

    property string name
    property string userName
    property alias iconSource: face.source
    property alias faceSize: face.width
    property alias notification: notificationText.text
    readonly property int padding: 3 //face.fixedMargins.top

    signal clicked()

    width: userItemWidth
    height: userItemHeight

    opacity: isCurrent ? 1.0 : 0.618

    Rectangle {//debug
        visible: debug
        border.color: "blue"
        border.width: 1
        anchors.fill: parent
        color: "#00000000"
        z:-1000
    }

    Behavior on opacity {
        NumberAnimation { duration: 250 }
    }

    Item {
        id: imageWrapper
        scale: isCurrent ? 1.0 : 0.8
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: padding*3
        }

        height: parent.height*2/3

        Behavior on scale {
            NumberAnimation {
                duration: 100
            }
        }

        //TODO there code was to show a blue border on mouseover
        //which shows that something is interactable.
        //we can't have that whilst using widgets/background as the base
        //I'd quite like it back

        PlasmaCore.FrameSvgItem {
            id: frame
            imagePath: "widgets/background"

            anchors {
                fill: face
                margins: -padding*3
            }
        }

        PlasmaCore.IconItem {
            id: face
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.top
            }
            height: width
        }
    }

    BreezeLabel {
        id: loginText
        anchors {
            bottom: notificationText.top
            left: parent.left
            right: parent.right
        }
        text: wrapper.name
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        maximumLineCount: 2
        wrapMode: Text.Wrap
        height: Math.round(Math.max(paintedHeight, theme.mSize(theme.defaultFont).height*1.2))
        Rectangle {//debug
            visible: debug
            border.color: "red"
            border.width: 1
            anchors.fill: parent
            color: "#00000000"
            z:-1000
        }
    }

    BreezeLabel {
        id: notificationText
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
            bottomMargin: padding
        }
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        maximumLineCount: 2
        wrapMode: Text.Wrap
        font.weight: Font.Bold
        height: Math.round(Math.max(paintedHeight, theme.mSize(theme.defaultFont).height*1.2))

        Rectangle {//debug
            visible: debug
            border.color: "yellow"
            border.width: 1
            anchors.fill: parent
            color: "#00000000"
            z:-1000
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onClicked: wrapper.clicked();
    }
}
