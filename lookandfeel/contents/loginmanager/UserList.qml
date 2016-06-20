/*
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
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

import QtQuick 2.2

ListView {
    id: view
    readonly property string selectedUser: currentItem ? currentItem.userName : ""
    readonly property int userItemWidth: units.gridUnit * 10
    readonly property int userItemHeight: units.gridUnit * 10
    readonly property int userFaceSize: units.gridUnit * 6

    implicitHeight: userItemHeight

    activeFocusOnTab : true

    /*
     * Signals that a user was explicitly selected
     */
    signal userSelected;

    orientation: ListView.Horizontal
    highlightRangeMode: ListView.StrictlyEnforceRange

    //centre align selected item (which implicitly centre aligns the rest
    preferredHighlightBegin: width/2 - userItemWidth/2
    preferredHighlightEnd: preferredHighlightBegin

    delegate: UserDelegate {
        name: model.realName || model.name
        userName: model.name
        iconSource: model.icon || ""

        onClicked: {
            ListView.view.currentIndex = index;
            ListView.view.userSelected();
        }
    }

    Keys.onEscapePressed: view.userSelected()
    Keys.onEnterPressed: view.userSelected()
    Keys.onReturnPressed: view.userSelected()
}
