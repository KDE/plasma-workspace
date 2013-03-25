/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
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

import QtQuick 1.1
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.qtextracomponents 0.1

MouseArea {
    id: main

    hoverEnabled: true
    onEntered: toolTipHideTimer.running = false
    onExited: toolTipHideTimer.running = true

    width: childrenRect.width
    height: 200

    property variant icon
    property string title
    property string description
    property string author
    property string email
    property string license
    property string pluginName
    property bool local

    onClicked: tooltipDialog.visible = false
    Connections {
        target: tooltipDialog
        onAppletDelegateChanged: {
            if (!tooltipDialog.appletDelegate) {
                return
            }
            icon = tooltipDialog.appletDelegate.icon
            title = tooltipDialog.appletDelegate.title
            description = tooltipDialog.appletDelegate.description
            author = tooltipDialog.appletDelegate.author
            email = tooltipDialog.appletDelegate.email
            license = tooltipDialog.appletDelegate.license
            pluginName = tooltipDialog.appletDelegate.pluginName
            local = tooltipDialog.appletDelegate.local
        }
    }
    QIconItem {
        id: tooltipIconWidget
        anchors.left: parent.left
        anchors.top: parent.top
        width: theme.hugeIconSize
        height: width
        icon: main.icon
    }
    Column {
        id: nameColumn
        spacing: 8
        anchors {
            left: tooltipIconWidget.right
            leftMargin: 8
            top: parent.top
            right: parent.right
        }

        PlasmaComponents.Label {
            text: title
            font.bold:true
            anchors.left: parent.left
            anchors.right: parent.right
            height: paintedHeight
            wrapMode: Text.Wrap
        }
        PlasmaComponents.Label {
            text: description
            anchors.left: parent.left
            anchors.right: parent.right
            wrapMode: Text.Wrap
        }
    }
    Grid {
        anchors.top: tooltipIconWidget.bottom
        anchors.topMargin: 16
        anchors.bottom: uninstallButton.top
        anchors.bottomMargin: 4
        rows: 3
        columns: 2
        spacing: 4
        PlasmaComponents.Label {
            text: i18n("License:")
        }
        PlasmaComponents.Label {
            id: licenseText
            text: license
            wrapMode: Text.Wrap
        }
        PlasmaComponents.Label {
            text: i18n("Author:")
        }
        PlasmaComponents.Label {
            text: author
            wrapMode: Text.Wrap
        }
        PlasmaComponents.Label {
            text: i18n("Email:")
        }
        PlasmaComponents.Label {
            text: email
        }
    }

    PlasmaComponents.Button {
        id: uninstallButton
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        opacity: local ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: 250 }
        }
        iconSource: "application-exit"
        text: i18n("Uninstall")
        onClicked: {
            widgetExplorer.uninstall(pluginName)
            tooltipDialog.visible = false
        }
    }
}
