/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
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
import QtQuick.Window 2.0

import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents

Item {
    id: root

    property Item applet
    property Item compactRepresentation


    onAppletChanged: {
        applet.parent = appletParent
        applet.anchors.fill = applet.parent
    }
    onCompactRepresentationChanged: {
        compactRepresentation.parent = root
        compactRepresentation.anchors.fill = root
    }


    PlasmaCore.Dialog {
        id: popupWindow
        //windowFlags: Qt.Popup
        color: Qt.rgba(0,0,0,0)
        visible: plasmoid.expanded
        onVisibleChanged: {
            if (!visible) {
                plasmoid.expanded = false
            }
        }
        //onWidthChanged: appletParent.width = width
        //onHeightChanged:appletParent.height = height
        mainItem: appletParent
        Rectangle {
            id: appletParent
            radius: 10
            width: 200//applet.implicitWidth
            height: 200//applet.implicitHeight
            onWidthChanged: applet.width = width
            onHeightChanged: applet.height = height
        }
    }
}
