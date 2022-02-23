/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.2

import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

FocusScope {
    id: root

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
     *
     * The following are also handled:
     *  - vtNumber
     *  - displayNumber
     *  - session
     *  - isTty
     */
    property alias userListModel: userListView.model

    /*
     * Self explanatory
     */
    property alias userListCurrentIndex: userListView.currentIndex
    property alias userListCurrentItem: userListView.currentItem
    property bool showUserList: true

    property alias userList: userListView

    property int fontSize: PlasmaCore.Theme.defaultFont.pointSize + 2

    default property alias _children: innerLayout.children

    // FIXME: move this component into a layout, rather than abusing
    // anchors and implicitly relying on other components' built-in
    // whitespace to avoid items being overlapped.
    UserList {
        id: userListView
        visible: showUserList && y > 0
        anchors {
            bottom: parent.verticalCenter
            // We only need an extra bottom margin when text is constrained,
            // since only in this case can the username label be a multi-line
            // string that would otherwise overflow.
            bottomMargin: constrainText ? PlasmaCore.Units.gridUnit * 3 : 0
            left: parent.left
            right: parent.right
        }
        fontSize: root.fontSize
    }

    //goal is to show the prompts, in ~16 grid units high, then the action buttons
    //but collapse the space between the prompts and actions if there's no room
    //ui is constrained to 16 grid units wide, or the screen
    ColumnLayout {
        id: prompts
        anchors.top: parent.verticalCenter
        anchors.topMargin: PlasmaCore.Units.gridUnit * 0.5
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        PlasmaComponents3.Label {
            id: notificationsLabel
            font.pointSize: root.fontSize
            Layout.maximumWidth: PlasmaCore.Units.gridUnit * 16
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.italic: true
        }
        ColumnLayout {
            Layout.minimumHeight: implicitHeight
            Layout.maximumHeight: PlasmaCore.Units.gridUnit * 10
            Layout.maximumWidth: PlasmaCore.Units.gridUnit * 16
            Layout.alignment: Qt.AlignHCenter
            ColumnLayout {
                id: innerLayout
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
            }
            Item {
                Layout.fillHeight: true
            }
        }
        Row { //deliberately not rowlayout as I'm not trying to resize child items
            id: actionItemsLayout
            spacing: PlasmaCore.Units.largeSpacing / 2
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
        }
        Item {
            Layout.fillHeight: true
        }
    }
}
