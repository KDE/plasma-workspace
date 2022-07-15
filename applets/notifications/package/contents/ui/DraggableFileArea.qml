/*
    SPDX-FileCopyrightText: 2016, 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.8

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.private.notifications 2.0 as Notifications

MouseArea {
    id: area

    signal activated
    signal contextMenuRequested(int x, int y)

    property Item dragParent
    property url dragUrl
    property var dragPixmap
    property int dragPixmapSize: PlasmaCore.Units.iconSizes.large

    readonly property bool dragging: Notifications.DragHelper.dragActive

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
        if (_pressX !== -1 && _pressY !== -1 && Notifications.DragHelper.isDrag(_pressX, _pressY, mouse.x, mouse.y)) {
            Notifications.DragHelper.dragPixmapSize = area.dragPixmapSize;
            Notifications.DragHelper.startDrag(area.dragParent, area.dragUrl, area.dragPixmap);
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
