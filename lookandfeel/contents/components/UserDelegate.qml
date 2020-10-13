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

import QtQuick 2.8
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.13 as Kirigami

Item {
    id: wrapper

    // If we're using software rendering, draw outlines instead of shadows
    // See https://bugs.kde.org/show_bug.cgi?id=398317
    readonly property bool softwareRendering: GraphicsInfo.api === GraphicsInfo.Software

    property bool isCurrent: true

    property string name
    property string userName
    property string avatarPath
    property string iconSource
    property bool constrainText: true
    property alias nameFontSize: usernameDelegate.font.pointSize
    property int fontSize: PlasmaCore.Theme.defaultFont.pointSize + 2
    signal clicked()

    property real faceSize: units.gridUnit * 7

    opacity: isCurrent ? 1.0 : 0.5

    Behavior on opacity {
        OpacityAnimator {
            duration: units.longDuration
        }
    }

    Kirigami.Avatar {
        id: avatar

        source: wrapper.avatarPath
        name: wrapper.name

        width: faceSize - PlasmaCore.Units.largeSpacing
        height: faceSize - PlasmaCore.Units.largeSpacing

        anchors {
            bottom: usernameDelegate.top
            bottomMargin: PlasmaCore.Units.largeSpacing
            horizontalCenter: parent.horizontalCenter
        }

        transitions: [
            Transition {
                from: "normal"
                to: "current"
                NumberAnimation {
                    properties: "width,height"
                    duration: PlasmaCore.Units.longDuration
                }
            },
            Transition {
                from: "current"
                to: "normal"
                NumberAnimation {
                    properties: "width,height"
                    duration: PlasmaCore.Units.shortDuration
                }
            }
        ]

        states: [
            State {
                name: "current"
                when: wrapper.isCurrent

                PropertyChanges {
                    target: avatar
                    width: faceSize
                    height: faceSize
                }
            },
            State {
                name: "normal"
                when: true
            }
        ]
    }

    PlasmaComponents3.Label {
        id: usernameDelegate
        font.pointSize: wrapper.fontSize
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        width: constrainText ? parent.width : implicitWidth
        text: wrapper.name
        style: softwareRendering ? Text.Outline : Text.Normal
        styleColor: softwareRendering ? PlasmaCore.ColorScope.backgroundColor : "transparent" //no outline, doesn't matter
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        //make an indication that this has active focus, this only happens when reached with keyboard navigation
        font.underline: wrapper.activeFocus
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
