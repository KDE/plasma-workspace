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

Item {
    property alias model: usersList.model
    property alias selectedUser: usersList.selectedUser
    property alias selectedIndex: usersList.currentIndex
    property alias delegate: usersList.delegate
    property alias notification: notificationLabel.text

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

        activeFocusOnTab: true

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
        //           / currentIndex: indexForUserName(greeter.lastLoggedInUser)
        cacheBuffer: 1000

        //highlight the item in the middle. The actual list view starts -1.5 userItemWidths so this moves the highlighted item to the centre
        preferredHighlightBegin: userItemWidth * 1
        preferredHighlightEnd: userItemWidth * 2

        //if the user presses down or enter, focus password
        //if user presses any normal key
        //copy that character pressed to the pasword box and force focus

        //can't use forwardTo as I want to switch focus. Also it doesn't work.
        Keys.onPressed: {
            if (event.key == Qt.Key_Down ||
                    event.key == Qt.Key_Enter ||
                    event.key == Qt.Key_Return) {
                passwordInput.forceActiveFocus();
            } else if (event.key & Qt.Key_Escape) {
                //if special key, do nothing. Qt.Escape is 0x10000000 which happens to be a mask used for all special keys in Qt.
            } else {
                passwordInput.text += event.text;
                passwordInput.forceActiveFocus();
            }
        }

        Component.onCompleted: {
            currentIndex = 0;
        }
    }

    BreezeLabel {
        id: notificationLabel
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }

        width: usersList.userItemWidth * 3 //don't pass the infoPane
        horizontalAlignment: Text.AlignHCenter
        maximumLineCount: 1
        wrapMode: Text.Wrap

        font.weight: Font.Bold
        height: Math.round(Math.max(paintedHeight, theme.mSize(theme.defaultFont).height*1.2))

    }
}
