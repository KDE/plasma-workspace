/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import "../osd"

PlasmaCore.FrameSvgItem {
    id: osd

    property alias timeout: osdItem.timeout
    property alias osdValue: osdItem.osdValue
    property alias osdMaxValue: osdItem.osdMaxValue
    property alias icon: osdItem.icon
    property alias showingProgress: osdItem.showingProgress

    objectName: "onScreenDisplay"
    visible: false
    width: osdItem.width + margins.left + margins.right
    height: osdItem.height + margins.top + margins.bottom
    imagePath: "widgets/background"

    function show() {
        osd.visible = true;
        hideAnimation.restart();
    }

    // avoid leaking ColorScope of lock screen theme into the OSD "popup"
    PlasmaCore.ColorScope {
        width: osdItem.width
        height: osdItem.height
        anchors.centerIn: parent
        colorGroup: PlasmaCore.Theme.NormalColorGroup

        OsdItem {
            id: osdItem
        }
    }

    SequentialAnimation {
        id: hideAnimation
        // prevent press and hold from flickering
        PauseAnimation {
            duration: PlasmaCore.Units.shortDuration
        }
        NumberAnimation {
            target: osd
            property: "opacity"
            from: 1
            to: 0
            duration: osd.timeout
            easing.type: Easing.InQuad
        }
        ScriptAction {
            script: {
                osd.visible = false;
                osd.opacity = 1;
                osd.icon = "";
                osd.osdValue = 0;
            }
        }
    }
}
