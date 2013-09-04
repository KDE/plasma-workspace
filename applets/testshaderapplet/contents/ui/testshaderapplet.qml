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
    width: 400
    height: 400

    property int minimumWidth: units.gridUnit * 20
    property int minimumHeight: units.gridUnit * 30
    property int _s: theme.iconSizes.small
    property int _h: theme.iconSizes.desktop
    property int _m: 12

    Item {
        id: mainItem
        anchors.fill: parent

        PlasmaComponents.TabBar {
            id: tabBar

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: theme.iconSizes.toolbar * 1.5

            PlasmaComponents.TabButton { tab: colorShower; text: tab.pageName; }
            PlasmaComponents.TabButton { tab: wobbleExample; text: tab.pageName; }
            PlasmaComponents.TabButton { tab: shadowExample; text: tab.pageName; }
            PlasmaComponents.TabButton { tab: simpleExample; text: tab.pageName; }
            //PlasmaComponents.TabButton { tab: vertexPage; iconSource: vertexPage.icon; }
        }

        PlasmaComponents.TabGroup {
            id: tabGroup
            anchors {
                left: parent.left
                right: parent.right
                top: tabBar.bottom
                bottom: parent.bottom
            }

            ColorShower {
                id: colorShower
            }
            WobbleExample {
                id: wobbleExample
            }
//             ColorExample {
//                 id: colorExample
//             }
            Shadows {
                id: shadowExample
            }
            SimpleExample {
                id: simpleExample
            }
//             EditorPage {
//                 id: vertexPage
//             }
        }
    }

    Component.onCompleted: {
        print("Shader Test Applet loaded");
    }
}