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
import QtQuick.Controls 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.plasma.workspace.components 2.0

PlasmaCore.ColorScope {
    id: root
    colorGroup: PlasmaCore.Theme.ComplementaryColorGroup
    width: 1600
    height: 900

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Repeater {
        model: screenModel

        Background {
            x: geometry.x
            y: geometry.y
            width: geometry.width
            height:geometry.height
            source: config.background
            fillMode: Image.PreserveAspectCrop
        }
    }

    Login {
        id: login
        sessionIndex: sessionButton.currentIndex

        anchors.top: parent.top
        anchors.topMargin: footer.height
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right

        searching: !config.usernamePrompt

        focus: true
    }

    //Footer
    RowLayout {
        id: footer
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: units.smallSpacing

        SessionButton {
            id: sessionButton
            Component.onCompleted: {
                currentIndex = sessionModel.lastIndex
            }
        }
        PlasmaComponents.ToolButton {
            implicitWidth: minimumWidth
            property alias searching : login.searching

            iconSource: searching ? "edit-select" : "search"
            text: searching ? i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Select User") :
                              i18nd("plasma_lookandfeel_org.kde.lookandfeel","Search for User")

            onClicked: {
                searching = !searching
            }
        }
        Item {
            Layout.fillWidth: true
        }

        PlasmaComponents.ToolButton {
            iconSource: "system-suspend"
            text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Suspend")
            implicitWidth: minimumWidth
            onClicked: sddm.suspend()
            visible: sddm.canSuspend
        }

        PlasmaComponents.ToolButton {
            iconSource: "system-reboot"
            text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Restart")
            implicitWidth: minimumWidth
            onClicked: sddm.reboot()
            visible: sddm.canReboot
        }

        PlasmaComponents.ToolButton {
            iconSource: "system-shutdown"
            text: i18nd("plasma_lookandfeel_org.kde.lookandfeel","Shutdown")
            implicitWidth: minimumWidth
            onClicked: sddm.powerOff()
            visible: sddm.canPowerOff
        }

        BatteryIcon {
            implicitWidth: units.iconSizes.medium
            implicitHeight: units.iconSizes.medium

            visible: pmSource.data["Battery"]["Has Cumulative"]

            hasBattery: true
            percent: pmSource.data["Battery"]["Percent"]
            pluggedIn: pmSource.data["AC Adapter"] ? pmSource.data["AC Adapter"]["Plugged in"] : false

            PlasmaCore.DataSource {
                id: pmSource
                engine: "powermanagement"
                connectedSources: ["Battery", "AC Adapter"]
            }
        }
        KeyboardButton { }
    }
}
