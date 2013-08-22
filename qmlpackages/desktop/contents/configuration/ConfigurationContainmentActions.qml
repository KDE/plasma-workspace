/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
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
import QtQuick.Controls 1.0 as QtControls
import QtQuick.Layouts 1.0


Item {
    id: root

    implicitWidth: childrenRect.width
    implicitHeight: childrenRect.height

    Column {
        anchors.centerIn: parent

        Repeater {
            model: 3
            delegate: RowLayout {
                width: root.width * 0.8
                QtControls.Button {
                    text: "Middle Button"
                }
                QtControls.ComboBox {
                    Layout.fillWidth: true
                    model: configDialog.containmentActionConfigModel
                    textRole: "name"
                }
                QtControls.Button {
                    iconName: "configure"
                    width: height
                }
                QtControls.Button {
                    iconName: "dialog-information"
                    width: height
                }
                QtControls.Button {
                    iconName: "list-remove"
                    width: height
                }
            }
        }
        QtControls.Button {
            text: "Add Action"
        }
    }
            
}
