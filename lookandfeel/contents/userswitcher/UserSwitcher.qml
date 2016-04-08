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

Item {
    id: root

    signal dismissed

    height: units.largeSpacing * 14
    width: screenGeometry.width

    SessionsModel {
        id: sessionsModel
        // the calls takes place asynchronously; if we were to dismiss the dialog right
        // after startNewSession/switchUser we would be destroyed before the reply
        // returned leaving us do nothing (Bug 356945)
        onStartedNewSession: root.dismissed()
        onSwitchedUser: root.dismissed()
    }

    Controls.Action {
        onTriggered: root.dismissed()
        shortcut: "Escape"
    }

    BreezeBlock {
        id: block
        anchors.fill: parent

        main: ColumnLayout {
            property alias selectedItem: usersSelection.selectedItem

            spacing: 0

            BreezeHeading {
                Layout.alignment: Qt.AlignHCenter
                text: i18ndc("plasma_lookandfeel_org.kde.lookandfeel","Title of dialog","Switch User")
            }

            BreezeHeading {
                Layout.fillWidth: true
                Layout.fillHeight: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","There are currently no other active sessions.")
                visible: sessionsModel.count === 0
            }

            UserSelect {
                id: usersSelection
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: sessionsModel.count > 0

                focus: true
                infoPaneVisible: false

                model: sessionsModel

                Controls.Action {
                    onTriggered: usersSelection.decrementCurrentIndex()
                    shortcut: "Left"
                }
                Controls.Action {
                    onTriggered: usersSelection.incrementCurrentIndex()
                    shortcut: "Right"
                }

                delegate: UserDelegate {
                    readonly property int vtNumber: model.vtNumber

                    name: {
                        var username = (model.realName || model.name)
                        if (model.isNewSession) {
                            return username // "new session" comes from the model already
                        } else if (model.isTty) {
                            return i18ndc("plasma_lookandfeel_org.kde.lookandfeel","Username (logged in on console)", "%1 (TTY)", username)
                        } else if (!model.name && !model.session) {
                            return i18ndc("plasma_lookandfeel_org.kde.lookandfeel","Unused session, nobody logged in there", "Unused")
                        } else if (model.displayNumber) {
                            return i18ndc("plasma_lookandfeel_org.kde.lookandfeel","Username (on display number)", "%1 (on Display %2)", username, model.displayNumber)
                        }

                        return username
                    }
                    iconSource: model.icon || "user-identity"
                    width: ListView.view.userItemWidth
                    faceSize: ListView.view.userFaceSize

                    onClicked: ListView.view.currentIndex = index
                }
            }
        }

        controls: Item {
            Layout.fillWidth: true
            height: buttons.height

            PlasmaComponents.Button {
                id: newSessionButton
                anchors.left: parent.left
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","New Session")
                visible: sessionsModel.canStartNewSession
                onClicked: {
                    sessionsModel.startNewSession(sessionsModel.shouldLock)
                }
            }

            Controls.Action {
                onTriggered: newSessionButton.clicked(null)
                shortcut: "Alt+N"
            }

            RowLayout {
                id: buttons
                anchors.centerIn: parent

                PlasmaComponents.Button {
                    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Cancel")
                    onClicked: root.dismissed()
                }
                PlasmaComponents.Button {
                    id: commitButton
                    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Switch")
                    visible: sessionsModel.count > 0
                    onClicked: {
                        sessionsModel.switchUser(block.mainItem.selectedItem.vtNumber, sessionsModel.shouldLock)
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
}
