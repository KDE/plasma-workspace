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

// ButtonsPage

PlasmaComponents.Page {
    id: plasmoidPage
    anchors {
        fill: parent
        margins: _s
    }
    Column {
        spacing: _s/2
        anchors.fill: parent
        PlasmaExtras.Heading {
            level: 1
            width: parent.width
            text: "Buttons"
        }
        Row {
            height: _h
            spacing: _s
            PlasmaComponents.Button {
                text: "Button"
                iconSource: "call-start"
            }
            PlasmaComponents.ToolButton {
                text: "ToolButton"
                iconSource: "call-stop"
            }
        }
        Row {
            height: _h
            spacing: _s
            PlasmaComponents.RadioButton {
                id: radio
                text: "RadioButton"
                //iconSource: "call-stop"
                onCheckedChanged: if (checked) tfield.forceActiveFocus()
            }
            PlasmaComponents.TextField {
                id: tfield
                enabled: radio.checked
                text: "input here"
                clearButtonShown: true
            }
        }
//         PlasmaComponents.TextArea {
//             width: parent.width
//             height: _h*2
//         }
    }
}

