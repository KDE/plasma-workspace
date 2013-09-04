/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.qtextracomponents 2.0 as QtExtras

Item {
    id: root
    width: 100
    height: 100
    clip: true
    property int minimumWidth: units.gridUnit * 20
    property int minimumHeight: units.gridUnit * 30

    property int _s: theme.iconSizes.small
    property int _h: theme.iconSizes.desktop

    PlasmaCore.DataSource {
        id: dataSource

    }

    PlasmaComponents.TabBar {
        id: tabBar

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: _h

        PlasmaComponents.TabButton { tab: dragPage; iconSource: "preferences-desktop-mouse"}
        PlasmaComponents.TabButton { tab: iconsPage; iconSource: "preferences-desktop-icons"}
        PlasmaComponents.TabButton { tab: dialogsPage; iconSource: "preferences-system-windows"}
        PlasmaComponents.TabButton { tab: buttonsPage; iconSource: "preferences-desktop-theme"}
        PlasmaComponents.TabButton { tab: plasmoidPage; iconSource: "plasma"}
        PlasmaComponents.TabButton { tab: mousePage; iconSource: "preferences-desktop-mouse"}
    }

    PlasmaComponents.TabGroup {
        id: tabGroup
        anchors {
            left: parent.left
            right: parent.right
            top: tabBar.bottom
            bottom: parent.bottom
        }

        //currentTab: tabBar.currentTab

        DragPage {
            id: dragPage
        }
        IconsPage {
            id: iconsPage
        }
        DialogsPage {
            id: dialogsPage
        }

        ButtonsPage {
            id: buttonsPage
        }


        PlasmoidPage {
            id: plasmoidPage
        }

        MousePage {
            id: mousePage
        }

    }

    Component.onCompleted: {
        print("Components Test Applet loaded");
        //dataSource.engine = "org.kde.foobar"
//         tabGroup.currentTab = mousePage;
    }
}