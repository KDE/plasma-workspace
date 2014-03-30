/*
    Copyright (C) 2011-2012  Lamarque Souza <Lamarque.Souza.ext@basyskom.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/**Documented API
Inherits:
        PlasmaCore.FrameSvgItem

Imports:
        QtQuick 2.0
        org.kde.plasma.core
        org.kde.kquickcontrolsaddons

Description:
        A button with label and icon (at the right) which uses the plasma theme.
        The button uses hover event to change the background on mouse over, supports tab stop
        and context menu.
        Plasma theme is the theme which changes via the systemsetting - workspace appearence
        - desktop theme.

Properties:
      * string text:
        This property holds the text label for the button.
        For example,the ok button has text 'ok'.
        The default value for this property is an empty string.

      * string iconSource:
        This property holds the source url for the Button's icon.
        The default value is an empty url, which displays no icon.

      * bool smallButton:
        Make the button use a different SVG element as background.
        The default is false.

      * bool menu:
        Indicates if the button will have a context menu. The menu is created by
        the parent.
        The default value is false.

       * ContextMenu contextMenu
        This property holds the contextMenu element.
        The default is a null Item.

      * Item tabStopNext:
        This property holds the next Item in a tab stop chain.

      * Item tabStopBack:
        This property holds the previous Item in a tab stop chain.

Signals:
      * onClicked:
        This handler is called when there is a click.
      * onPressed:
        This handler is called when there is a press.
      * onPressAndHold:
        This handler is called when there is a press and hold.
**/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kquickcontrolsaddons 2.0

PlasmaCore.FrameSvgItem {
    id: button
    property string text
    property string iconSource
    property bool smallButton: false
    property bool menu: false
    property ContextMenu contextMenu
    property Item tabStopNext
    property Item tabStopBack
    property int accelKey: -1
    height: theme.desktopFont.mSize.height + 22

    signal clicked()
    signal pressed()
    signal pressAndHold()

//     PlasmaCore.Theme {
//         id: theme
//     }

    PlasmaCore.SvgItem {
        id: background
        anchors.fill: parent

        svg: PlasmaCore.Svg {
            imagePath: "dialogs/shutdowndialog"
        }
        elementId: "button-normal"
    }

    Text {
        id: labelElement
        font.pointSize: theme.desktopFont.pointSize
        color: theme.textColor
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: theme.mSize(theme.defaultFont).width
        }

        onPaintedWidthChanged: {
            button.width = Math.max(button.width, theme.mSize(theme.defaultFont).width + labelElement.width + 2*theme.mSize(theme.defaultFont).width + iconElement.width + theme.mSize(theme.defaultFont).width)
        }
    }

    // visual part of the label accelerator implementation. See main.qml (Keys.onPressed) to see the code
    // that actually triggers the action.
    onTextChanged: {
        var i = button.text.indexOf('&')

        if (i > -1) {
            var stringToReplace = button.text.substr(i, 2)
            accelKey = stringToReplace.toUpperCase().charCodeAt(1)
            labelElement.text = button.text.replace(stringToReplace, '<u>'+stringToReplace[1]+'</u>')
        } else {
            labelElement.text = button.text
        }
    }

    QIconItem {
        id: menuIconElement

        // if textColor is closer to white than to black use "draw-triangle4", which is also close to white.
        // Otherwise use "arrow-down", which is green. I have not found a black triangle icon.
        icon: theme.textColor > "#7FFFFF" ? QIcon("draw-triangle4") : QIcon("arrow-down")

        width: 6
        height: width
        visible: button.menu

        anchors {
            right: iconElement.left
            rightMargin: 2
            bottom: parent.bottom
            bottomMargin: 2
        }
    }

    QIconItem {
        id: iconElement
        icon: QIcon(iconSource)
        width: height
        height: parent.height - 8

        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: 4
        }

        MouseArea {
            z: 10
            anchors.fill: parent
            onClicked: {
                button.focus = true
                if (menu) {
                    button.pressAndHold()
                } else {
                    button.clicked()
                }
            }
        }
    }

    Component.onCompleted: {
        if (button.focus) {
            background.elementId = button.smallButton ? "button-small-hover" : "button-hover"
        } else {
            background.elementId = button.smallButton ? "button-small-normal" : "button-normal"
        }
        if (button.smallButton) {
            height = theme.desktopFont.mSize.height + 12
        } else {
            height = theme.desktopFont.mSize.height + 22
        }
    }

    onActiveFocusChanged: {
        if (activeFocus) {
            background.elementId = button.smallButton ? "button-small-hover" : "button-hover"
            //console.log("KSMButton.qml activeFocus "+activeFocus+" "+button.text)
        } else {
            background.elementId = button.smallButton ? "button-small-normal" : "button-normal"
        }
    }

    onTabStopNextChanged: {
        KeyNavigation.tab = tabStopNext
        KeyNavigation.down = tabStopNext
        KeyNavigation.right = tabStopNext
    }

    onTabStopBackChanged: {
        KeyNavigation.backtab = tabStopBack
        KeyNavigation.up = tabStopBack
        KeyNavigation.left = tabStopBack
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_Return ||
            event.key == Qt.Key_Enter) {
            mouseArea.clicked(null)
        } else if (event.key == Qt.Key_Space) {
            button.pressAndHold();
        }
    }

    MouseArea {
        id: mouseArea

        z: -10
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            button.focus = true
            button.clicked()
        }
        onPressed: button.pressed()
        onPressAndHold: button.pressAndHold()
        onEntered: {
            background.elementId = button.smallButton ? "button-small-hover" : "button-hover"
        }
        onExited: {
            if (!button.focus) {
                background.elementId = button.smallButton ? "button-small-normal" : "button-normal"
            }
        }
    }
}
