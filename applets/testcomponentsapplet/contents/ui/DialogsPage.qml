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
import QtQuick.Window 2.0

import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.qtextracomponents 0.1 as QtExtras

// DialogsPage

PlasmaComponents.Page {
    id: dialogsPage
    anchors {
        fill: parent
        margins: _s
    }
    Column {
        spacing: _s/2
        anchors.fill: parent
        PlasmaExtras.Title {
            width: parent.width
            text: "Dialogs"
        }
        Row {
            height: _h
            spacing: _s
            PlasmaComponents.Button {
                text: "PlasmaCore.Dialog"
                iconSource: "dialog-ok-apply"
                checkable: true
                onCheckedChanged: pcDialog.visible = checked
            }
            PlasmaComponents.Label {
                text: pcDialog.visible ? "shown" : "hidden"
            }

            PlasmaCore.Dialog {
                id: pcDialog
                windowFlags: Qt.Popup
                mainItem: dContent2
                DialogContent {
                    id: dContent2
                    onCloseMe: {
                        pcDialog.close()
                        //pcDialog.visible = false
                    }
                }
            }
        }
        Row {
            height: _h
            spacing: _s
            PlasmaComponents.Button {
                id: radio
                checkable: true
                iconSource: "dialog-ok"
                text: "QtQuick2.Window"
            }
            Window {
                id: qWindow
                visible: radio.checked
                width: childrenRect.width
                height: childrenRect.height
                color: Qt.rgba(0,0,0,0)
//                 Column {
//                     width: dialogsPage.width/2
//                     PlasmaComponents.TextArea {
//                         //anchors { left: parent.left; right: parent.right; top: parent.top; }
//                         width: parent.width
//                         height: _h*2
//                     }
//
//                     PlasmaComponents.Button {
//                         id: thanks
//                         iconSource: "dialog-ok"
//                         text: "Thanks."
//                         onClicked: selectionDialog.visible = false;
//                     }
//                 }
                DialogContent {
                    id: dContent
                    onCloseMe: {
                        qWindow.visible = false
                    }
                }
            }

            PlasmaComponents.Label {
                text: qWindow.visible ? "shown" : "hidden"
            }
        }
    }
}

