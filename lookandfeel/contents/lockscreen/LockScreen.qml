/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
    property int interfaceVersion: org_kde_plasma_screenlocker_greeter_interfaceVersion ? org_kde_plasma_screenlocker_greeter_interfaceVersion : 0
    signal clearPassword()

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    
    implicitWidth: 800
    implicitHeight: 600

    Loader {
        id: mainLoader
        anchors.fill: parent
        opacity: 0
        onItemChanged: opacity = 1

        focus: true

        Behavior on opacity {
            OpacityAnimator {
                duration: PlasmaCore.Units.longDuration
                easing.type: Easing.InCubic
            }
        }
    }
    Connections {
        id:loaderConnection
        target: org_kde_plasma_screenlocker_greeter_view
        function onFrameSwapped() {
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
