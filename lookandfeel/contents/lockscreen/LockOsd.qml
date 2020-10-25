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
        PauseAnimation { duration: 100 }
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
