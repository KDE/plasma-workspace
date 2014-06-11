/*
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
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

ListView {

    property int userItemWidth: 140
    property int userItemHeight: 140
    property int userFaceSize: 88
    property int padding: 4

    delegate: userDelegate
    focus: true

    orientation: ListView.Horizontal

    highlightRangeMode: ListView.StrictlyEnforceRange

    readonly property string selectedUser: currentItem.userName //FIXME read only

    Component {
        id: userDelegate

        Item {
            id: wrapper

            property bool isCurrent: ListView.isCurrentItem

            property string name: (model.realName === "") ? model.name : model.realName
            property string userName: model.name

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
                NumberAnimation {
                    duration: 250
                }
            }

            Item {
                id: imageWrapper
                scale: isCurrent ? 1.0 : 0.8
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                height: frame.height

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
                    width: face.width + padding * 6
                    height: face.height + padding * 6
                    imagePath: "widgets/background"

                    anchors.horizontalCenter: parent.horizontalCenter
                }

                PlasmaCore.IconItem {
                    id: face
                    anchors.centerIn: frame
                    width: userFaceSize
                    height: userFaceSize
                    source: model.icon ? model.icon : "user-identity"
                }
            }

            BreezeLabel {
                id: loginText
                anchors.top: imageWrapper.bottom
                anchors.topMargin: -10
                anchors.left: parent.left
                anchors.right: parent.right
                text: name
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 2
                wrapMode: Text.Wrap
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    wrapper.ListView.view.currentIndex = index;
                    wrapper.ListView.view.forceActiveFocus();
                }
            }
        }
    }

}
