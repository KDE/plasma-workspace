/*
 *   Copyright 2016,2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.8

MouseArea {
    id: area

    signal activated
    signal contextMenuRequested(int x, int y)

    property Item dragParent
    property url dragUrl
    property var dragPixmap

    readonly property bool dragging: plasmoid.nativeInterface.dragActive

    property int _pressX: -1
    property int _pressY: -1

    preventStealing: true
    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    acceptedButtons: Qt.LeftButton | Qt.RightButton

    onClicked: {
        if (mouse.button === Qt.LeftButton) {
            area.activated();
        }
    }
    onPressed: {
        if (mouse.button === Qt.LeftButton) {
            _pressX = mouse.x;
            _pressY = mouse.y;
        } else if (mouse.button === Qt.RightButton) {
            area.contextMenuRequested(mouse.x, mouse.y);
        }
    }
    onPositionChanged: {
        if (_pressX !== -1 && _pressY !== -1 && plasmoid.nativeInterface.isDrag(_pressX, _pressY, mouse.x, mouse.y)) {
            plasmoid.nativeInterface.startDrag(area.dragParent, area.dragUrl, area.dragPixmap);
            _pressX = -1;
            _pressY = -1;
        }
    }
    onReleased: {
        _pressX = -1;
        _pressY = -1;
    }
    onContainsMouseChanged: {
        if (!containsMouse) {
            _pressX = -1;
            _pressY = -1;
        }
    }
}
