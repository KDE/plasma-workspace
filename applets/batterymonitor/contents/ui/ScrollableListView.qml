/*
 *   Copyright 2012 Daniel Nicoletti <dantti12@gmail.com>
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
import org.kde.plasma.components 2.0 as Components
import org.kde.kquickcontrolsaddons 2.0

FocusScope {
    property alias delegate: list.delegate
    property alias model: list.model
    property alias view: list
    property alias interactive: list.interactive
    property alias currentIndex: list.currentIndex
    signal countChanged()

    Component.onCompleted: {
        list.countChanged.connect(countChanged)
    }

    ListView {
        id: list
        clip: true
        focus: true
        anchors {
            left:   parent.left
            right:  scrollBar.visible ? scrollBar.left : parent.right
            top :   parent.top
            bottom: parent.bottom
        }
        boundsBehavior: Flickable.StopAtBounds
    }
    Components.ScrollBar {
        id: scrollBar
        flickableItem: list
        anchors {
            right: parent.right
            top: list.top
            bottom: list.bottom
        }
    }
}
