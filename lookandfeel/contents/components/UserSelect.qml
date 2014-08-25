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
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

FocusScope {
    id: root
    property alias model: usersList.model
    property alias selectedUser: usersList.selectedUser
    property alias selectedIndex: usersList.currentIndex
    property alias delegate: usersList.delegate
    property alias notification: notificationLabel.text

    activeFocusOnTab: true

    function incrementCurrentIndex() {
        usersList.incrementCurrentIndex();
    }

    function decrementCurrentIndex() {
        usersList.decrementCurrentIndex()
    }

    InfoPane {
        id: infoPane
        anchors {
            verticalCenter: usersList.verticalCenter
            right: usersList.left
            left: parent.left
        }
    }

    UserList {
        id: usersList

        focus: true

        Rectangle {//debug
            visible: debug
            border.color: "red"
            border.width: 1
            anchors.fill: parent
            color: "#00000000"
            z:-1000
        }

        anchors {
            top: parent.top
            left: parent.horizontalCenter
            right: parent.right

            leftMargin: -userItemWidth*1.5 //allow 1 item to the left of the centre (the half is to fit the item that will go in the centre)
        }
        clip: true
        height: userItemHeight
        cacheBuffer: 1000

        //highlight the item in the middle. The actual list view starts -1.5 userItemWidths so this moves the highlighted item to the centre
        preferredHighlightBegin: userItemWidth * 1
        preferredHighlightEnd: userItemWidth * 2

        onUserSelected: {
            nextItemInFocusChain().forceActiveFocus();
        }
    }

    BreezeLabel {
        id: notificationLabel
        anchors {
            top: usersList.bottom
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            margins: units.largeSpacing
        }

        width: usersList.userItemWidth * 3 //don't pass the infoPane
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        maximumLineCount: 1
        wrapMode: Text.Wrap

        font.weight: Font.Bold
    }
}
