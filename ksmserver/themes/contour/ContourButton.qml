/*
 *   Copyright 2011-2012 Lamarque V. Souza <Lamarque.Souza.ext@basyskom.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**Documented API
Inherits:
        PlasmaCore.FrameSvgItem

Imports:
        QtQuick 2.0
        org.kde.plasma.core
        org.kde.kquickcontrolsaddons

Description:
        A simple button with label at the bottom and icon at the top which uses the plasma theme.
        Plasma theme is the theme which changes via the systemsetting - workspace appearence
        - desktop theme.

Properties:
      * string text:
        This property holds the text label for the button.
        For example, the ok button has text 'ok'.
        The default value for this property is an empty string.

      * font font:
        This property holds the font used by the button label.
        See also Qt documentation for font type.

      * string iconSource:
        This property holds the source url for the Button's icon.
        The default value is an empty url, which displays no icon.

      * int iconSize:
        This property holds the icon size.
        The default is use the natural image size.
Signals:
      * onClicked:
        This handler is called when there is a click.
**/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kquickcontrolsaddons 2.0

PlasmaCore.FrameSvgItem {
    id: button
    width: mainColumn.width
    height: mainColumn.height

    property alias text: labelElement.text
    property alias font: labelElement.font
    property string iconSource
    property alias iconSize: iconElement.width

    signal clicked()

    PlasmaCore.Theme {
        id: theme
    }

    Column {
        id: mainColumn

        QIconItem {
            id: iconElement
            icon: QIcon(iconSource)
            height: width

            MouseArea {
                anchors.fill: parent
                onClicked: button.clicked()
                onPressed: button.state = "Pressed"
                onReleased: button.state = "Normal"
            }
        }
        Text {
            id: labelElement
            anchors.horizontalCenter: iconElement.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            color: theme.textColor
            // Use theme.defaultFont in plasma-mobile and
            // theme.font in plasma-desktop.
            font.family: theme.defaultFont.family
            font.bold: theme.defaultFont.bold
            font.capitalization: theme.defaultFont.capitalization
            font.italic: theme.defaultFont.italic
            font.weight: theme.defaultFont.weight
            font.underline: theme.defaultFont.underline
            font.wordSpacing: theme.defaultFont.wordSpacing
        }
    }

    states: [
        State {
            name: "Normal"
            PropertyChanges { target: mainColumn; scale: 1.0}
        },
        State {
            name: "Pressed"
            PropertyChanges { target: mainColumn; scale: 0.9}
        }
    ]

    transitions: [
        Transition {
            NumberAnimation { properties: "scale"; duration: 50 }
        }
    ]
}
