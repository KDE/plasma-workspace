/***************************************************************************
 *   Copyright (C) 2015 Kai Uwe Broulik <kde@privat.broulik.de>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1 as Controls

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

import org.kde.plasma.private.sessions 2.0

import "../components"

PlasmaCore.ColorScope {
    id: root
    colorGroup: PlasmaCore.Theme.ComplementaryColorGroup

    signal dismissed
    signal ungrab

    height:screenGeometry.height
    width: screenGeometry.width

    Rectangle {
        anchors.fill: parent
        color: PlasmaCore.ColorScope.backgroundColor
        opacity: 0.5
    }

    SessionsModel {
        id: sessionsModel
        showNewSessionEntry: true

        // the calls takes place asynchronously; if we were to dismiss the dialog right
        // after startNewSession/switchUser we would be destroyed before the reply
        // returned leaving us do nothing (Bug 356945)
        onStartedNewSession: root.dismissed()
        onSwitchedUser: root.dismissed()

        onAboutToLockScreen: root.ungrab()
    }

    Controls.Action {
        onTriggered: root.dismissed()
        shortcut: "Escape"
    }

    Clock {
        anchors.bottom: parent.verticalCenter
        anchors.bottomMargin: units.gridUnit * 13
        anchors.horizontalCenter: parent.horizontalCenter
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.dismissed()
    }

    SessionManagementScreen {
        id: block
        anchors.fill: parent

        userListModel: sessionsModel

        RowLayout {
            PlasmaComponents.Button {
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Cancel")
                onClicked: root.dismissed()
            }
            PlasmaComponents.Button {
                id: commitButton
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Switch")
                visible: sessionsModel.count > 0
                onClicked: {
                    sessionsModel.switchUser(block.userListCurrentModelData.vtNumber, sessionsModel.shouldLock)
                }

                Controls.Action {
                    onTriggered: commitButton.clicked()
                    shortcut: "Return"
                }
                Controls.Action {
                    onTriggered: commitButton.clicked()
                    shortcut: "Enter" // on numpad
                }
            }
        }
    }
}
