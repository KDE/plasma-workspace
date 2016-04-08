/********************************************************************
 This file is part of the KDE project.

Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import org.kde.plasma.components 2.0 as PlasmaComponents
import "../components"

BreezeBlock {
    id: selectSessionBlock

    Action {
        onTriggered: stackView.pop()
        shortcut: "Escape"
    }

    main: UserSelect {
        id: sessionSelect

        model: sessionsModel
        delegate: UserDelegate {
            // so the button can access it from outside through currentItem
            readonly property int userVt: model.vtNumber

            name: {
                if (!model.session) {
                    return i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Nobody logged in on that session", "Unused")
                }

                var displayName = model.realName || model.name

                var location = ""
                if (model.isTty) {
                    location = i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "User logged in on console number", "TTY %1", model.vtNumber)
                } else if (model.displayNumber) {
                    location = i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "User logged in on console (X display number)", "on TTY %1 (Display %2)", model.vtNumber, model.displayNumber)
                }

                if (location) {
                    return i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Username (location)", "%1 (%2)", displayName, location)
                }
                return displayName
            }
            userName: model.name
            iconSource: model.icon || "user-identity"
            width: ListView.view.userItemWidth
            height: ListView.view.userItemHeight
            faceSize: ListView.view.userFaceSize

            onClicked: {
                ListView.view.currentIndex = index;
                ListView.view.forceActiveFocus();
            }
        }
    }

    controls: Item {
        height: childrenRect.height
        RowLayout {
            anchors.centerIn: parent
            PlasmaComponents.Button {
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Cancel")
                onClicked: stackView.pop()
            }
            PlasmaComponents.Button {
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Change Session")
                onClicked: {
                    sessionsModel.switchUser(selectSessionBlock.mainItem.selectedItem.userVt)
                    stackView.pop()
                    userSelect.selectedIndex = 0
                }
            }
        }
    }
}
