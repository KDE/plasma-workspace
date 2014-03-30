/*
*   Copyright (C) 2011 by Marco Martin <mart@kde.org>
*   Copyright (C) 2011-2012 by Lamarque V. Souza <Lamarque.Souza.ext@basyskom.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License as
*   published by the Free Software Foundation; either version 2, or
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

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: root

    property Item visualParent
    property int status: PlasmaComponents.DialogStatus.Closed
    signal clicked(int index)
    signal closeMenus()

    function append(dict)
    {
        listModel.append(dict)
    }

    function open()
    {
        var parent = root.visualParent ? root.visualParent : root.parent
        var pos = dialog.popupPosition(parent, Qt.alignCenter)
        dialog.x = pos.x
        dialog.y = pos.y

        dialog.visible = true
        dialog.activateWindow()
    }

    function close()
    {
        dialog.visible = false
    }

    visible: false

    ListModel {
         id: listModel
    }

    PlasmaCore.Dialog {
        id: dialog
        visible: false
//         windowFlags: Qt.Popup
        onVisibleChanged: {
            if (visible) {
                status = PlasmaComponents.DialogStatus.Open
            } else {
                status = PlasmaComponents.DialogStatus.Closed
            }
        }

        mainItem: Item {
            id: contentItem

            width: listView.width
            height: Math.min(listView.contentHeight, theme.mSize(theme.defaultFont).height * 25)

            ListView {
                id: listView
                anchors.fill: parent

                currentIndex : -1
                clip: true

                model: listModel
                delegate: MenuItem {
                    id: menuItem

                    text: itemText
                    index: itemIndex
                    subMenu: itemSubMenu != null
                    allowAmpersand: itemAllowAmpersand

                    Component.onCompleted: {
                        contentItem.width = Math.max(contentItem.width, menuItem.implicitWidth)
                        if (itemSubMenu) {
                            itemSubMenu.visualParent = menuItem
                            itemSubMenu.closeMenus.connect(root.close)
                        }
                    }
                    onClicked: {
                        if (itemSubMenu) {
                            //console.log("opening submenu")
                            itemSubMenu.open()
                        } else {
                            //console.log("emiting clicked(" + index + ")")
                            root.clicked(index)
                            root.close()
                        }
                    }
                }
            }
        }
    }

    onStatusChanged: {
        if (status == PlasmaComponents.DialogStatus.Opening) {
            if (listView.currentItem != null) {
                listView.currentItem.focus = false
            }
            listView.currentIndex = -1
            listView.positionViewAtIndex(0, ListView.Beginning)
        }
        else if (status == PlasmaComponents.DialogStatus.Open) {
            listView.focus = true
        }
        else if (status == PlasmaComponents.DialogStatus.Closed) {
            closeMenus()
        }
    }
}
