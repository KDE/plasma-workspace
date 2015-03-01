/***************************************************************************
 *   Copyright (C) 2014 by Aleix Pol Gonzalez <aleixpol@blue-systems.com>  *
 *   Copyright (C) 2014 by Marco Martin <mart@kde.org>                     *
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

import QtQuick 2.1
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents


PlasmaComponents.ButtonRow {
    id: root
    spacing: 0

    property bool canReboot
    property bool canShutdown
    property bool canLogout
    property string mode
    onModeChanged: {
        switch (mode) {
        case "reboot":
            restartButton.checked = true;
            break;
        case "shutdown":
            shutdownButton.checked = true;
            break;
        case "logout":
            logoutButton.checked = true;
            break;
        default:
            restartButton.checked = false;
            shutdownButton.checked = false;
            logoutButton.checked = false;
            break;
        }
    }

    //Don't show the buttons if there's nothing to click
    visible: !exclusive || (canReboot+canShutdown+canLogout)>1

    PlasmaComponents.ToolButton {
        id: restartButton
        flat: false
        iconSource: "system-reboot"
        visible: root.canReboot
        checkable: true
        Accessible.name: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Button to restart the computer", "Reboot")
        activeFocusOnTab: true

        onClicked: {
            root.mode = "reboot"
        }
    }

    PlasmaComponents.ToolButton {
        id: shutdownButton
        flat: false
        iconSource: "system-shutdown"
        visible: root.canShutdown
        checkable: true
        Accessible.name: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Button to shut down the computer", "Shutdown")
        activeFocusOnTab: true

        onClicked: {
            root.mode = "shutdown"
        }

    }

    PlasmaComponents.ToolButton {
        id: logoutButton
        flat: false
        iconSource: "system-log-out"
        visible: root.canLogout
        checkable: true
        Accessible.name: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Button to log out of the desktop session", "Log out")
        activeFocusOnTab: true

        onClicked: {
            root.mode = "logout"
        }

    }
}

