/*
 *   Copyright 2016 David Edmundson <davidedmundson@kde.org>
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
import QtQuick.Controls 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: root

    /*
     * Whether a text box for the user to type a username should be visible
     */
    property bool showUsernamePrompt: false
    /*
     * Any message to be displayed to the user, visible above the text fields
     */
    property alias notificationMessage: notificationsLabel.text

    /*
     * A list of Items (typically ActionButtons) to be shown in a Row beneath the prompts
     */
    property alias actionItems: actionItemsLayout.children

    /*
     * A model with a list of users to show in the view
     * The following roles should exist:
     *  - name
     *  - iconSource
     */
    property alias userListModel: userList.model

    /*
     * Self explanatory
     */
    property alias userListCurrentIndex: userList.currentIndex


    /*
     * Login has been requested with the following username and password
     * If username field is visible, it will be taken from that, otherwise from the "name" property of the currentIndex
     */
    signal loginRequest(string username, string password)

    function startLogin() {
        var username = showUsernamePrompt ? userNameInput.text : userList.selectedUser
        var password = passwordBox.text
        loginRequest(username, password);
    }

    UserList {
        id: userList
        anchors {
            bottom: parent.verticalCenter
            left: parent.left
            right: parent.right
        }
        onUserSelected: passwordBox.forceActiveFocus()
    }

    ColumnLayout {
        id: prompts
        anchors.top: parent.verticalCenter
        anchors.topMargin: units.gridUnit * 0.5
        anchors.horizontalCenter: parent.horizontalCenter

        height: Math.max(implicitHeight, units.gridUnit * 10)
        width: Math.max(implicitWidth, units.gridUnit * 16)

        PlasmaComponents.Label {
            id: notificationsLabel

            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.italic: true
        }

        PlasmaComponents.TextField {
            id: userNameInput
            Layout.fillWidth: true

            visible: showUsernamePrompt
            focus: showUsernamePrompt //if there's a username prompt it gets focus first, otherwise password does
            placeholderText: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Username");
        }

        PlasmaComponents.TextField {
            id: passwordBox
            Layout.fillWidth: true

            placeholderText: "Password"
            focus: !showUsernamePrompt
            echoMode: TextInput.Password

            onAccepted: startLogin()

            Keys.onEscapePressed: {
                mainStack.currentItem.forceActiveFocus();
            }

            //if empty and left or right is pressed change selection in user switch
            //this cannot be in keys.onLeftPressed as then it doesn't reach the password box
            Keys.onPressed: {
                if (event.key == Qt.Key_Left && !text) {
                    userList.decrementCurrentIndex();
                    event.accepted = true
                }
                if (event.key == Qt.Key_Right && !text) {
                    userList.incrementCurrentIndex();
                    event.accepted = true
                }
            }
        }
        PlasmaComponents.Button {
            id: loginButton
            Layout.fillWidth: true

            text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Login")
            onClicked: startLogin();
        }
        Item {
            Layout.fillHeight: true
        }
    }

    Row { //deliberately not rowlayout as I'm not trying to resize child items
        id: actionItemsLayout
        spacing: units.smallSpacing

        //align centre, but cap to the width of the screen
        anchors {
            top: prompts.bottom
            topMargin: units.smallSpacing
            horizontalCenter: parent.horizontalCenter
        }
    }
}
