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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0

import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.qtextracomponents 0.1 as QtExtras

// IconTab

PlasmaComponents.Page {
    id: iconsPage
    anchors {
        fill: parent
        margins: _s
    }
    Column {
        anchors.fill: parent
        spacing: _s

        PlasmaExtras.Title {
            width: parent.width
            elide: Text.ElideRight
            text: "This is a <i>PlasmaComponent</i>"
        }
        PlasmaComponents.Label {
            width: parent.width
            text: "Icons"
        }
        Row {
            height: _h
            spacing: _s

            PlasmaCore.IconItem {
                source: "configure"
                width: parent.height
                height: width
            }
            PlasmaCore.IconItem {
                source: "dialog-ok"
                width: parent.height
                height: width
            }

            PlasmaCore.IconItem {
                source: "maximize"
                width: parent.height
                height: width
            }


            PlasmaCore.IconItem {
                source: "akonadi"
                width: parent.height
                height: width
            }
            PlasmaCore.IconItem {
                source: "clock"
                width: parent.height
                height: width
            }
            QtExtras.QIconItem {
                icon: "preferences-desktop-icons"
                width: parent.height
                height: width
            }

        }
        PlasmaExtras.Heading {
            level: 4
            width: parent.width
            text: "Buttons"
        }
        Column {
            width: parent.width
            spacing: _s

            PlasmaComponents.Button {
                text: "Button"
                iconSource: "call-start"
            }
            PlasmaComponents.ToolButton {
                text: "ToolButton"
                iconSource: "call-stop"
            }
            PlasmaComponents.RadioButton {
                text: "RadioButton"
                //iconSource: "call-stop"
            }
        }

    }
}
