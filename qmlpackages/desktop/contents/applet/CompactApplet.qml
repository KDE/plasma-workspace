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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Window 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: root

    property int minimumWidth: compactRepresentation && compactRepresentation.minimumWidth !== undefined ? compactRepresentation.minimumWidth : -1
    property int minimumHeight: compactRepresentation && compactRepresentation.minimumHeight !== undefined ? compactRepresentation.minimumHeight : -1

    property int maximumWidth: compactRepresentation && compactRepresentation.maximumWidth !== undefined ? compactRepresentation.maximumWidth : -1
    property int maximumHeight: compactRepresentation && compactRepresentation.maximumHeight !== undefined ? compactRepresentation.maximumHeight : -1

    property int implicitWidth: compactRepresentation && compactRepresentation.implicitWidth !== undefined ? compactRepresentation.implicitWidth : -1
    property int implicitHeight: compactRepresentation && compactRepresentation.implicitHeight !== undefined ? compactRepresentation.implicitHeight : -1

    property bool fillWidth: compactRepresentation && compactRepresentation.fillWidth !== undefined ? compactRepresentation.fillWidth : false
    property bool fillHeight: compactRepresentation && compactRepresentation.fillHeight !== undefined ? compactRepresentation.fillHeight : false


    property Item applet
    property Item compactRepresentation

    onAppletChanged: {

        //if the applet size was restored to a stored size, or if is dragged from the desktop, restore popup size
        if (applet.width > 0) {
            popupWindow.mainItem.width = applet.width;
        }
        if (applet.height > 0) {
            popupWindow.mainItem.height = applet.height;
        }

        applet.parent = appletParent;
        applet.anchors.fill = applet.parent;
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
        visualParent: root
        mainItem: Item {
            id: appletParent

            width: applet && applet.implicitHeight > 0 ? applet.implicitHeight : theme.mSize(theme.defaultFont).width * 35
            height: applet && applet.implicitHeight > 0 ? applet.implicitHeight : theme.mSize(theme.defaultFont).height * 25
        }

        onActiveWindowChanged: {
            if (!activeWindow) {
               // plasmoid.expanded = false
            }
        }
        onVisibleChanged: {
            if (!visible) {
                plasmoid.expanded = false
            }
        }
    }
}
