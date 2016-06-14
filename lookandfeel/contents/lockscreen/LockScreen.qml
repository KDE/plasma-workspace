/********************************************************************
 This file is part of the KDE project.

Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

import QtQuick 2.5
import QtQuick.Controls 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.private.sessions 2.0
import "../components"

Item {
    id: root
    property bool viewVisible: false
    property bool debug: false
    property string notification
    property UserSelect userSelect: null
    property int interfaceVersion: org_kde_plasma_screenlocker_greeter_interfaceVersion ? org_kde_plasma_screenlocker_greeter_interfaceVersion : 0
    signal clearPassword()

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#1fb4f9"
            }
            GradientStop {
                position: 1.0
                color: "#197cf1"
            }
        }
        visible: image.status == Image.Null || image.status == Image.Error
    }

    Image {
        id: image
        anchors.fill: parent
        source: backgroundPath
        fillMode: Image.PreserveAspectCrop
    }

    Loader {
        id: mainLoader
        anchors.fill: parent
        opacity: 0
        onItemChanged: opacity = 1

        Behavior on opacity {
            OpacityAnimator {
                duration: units.longDuration
                easing.type: Easing.InCubic
            }
        }
    }
    Connections {
        id:loaderConnection
        target: org_kde_plasma_screenlocker_greeter_view
        onFrameSwapped: {
            mainLoader.source = "LockScreenUi.qml";
            loaderConnection.target = null;
        }
    }
    Component.onCompleted: {
        if (root.interfaceVersion < 2) {
            mainLoader.source = "LockScreenUi.qml";
        }
    }
}
