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

import org.kde.qtextracomponents 2.0

Item {
    id: root

    implicitWidth: childrenRect.width
    implicitHeight: childrenRect.height

    function saveConfig() {
        configDialog.currentContainmentActionsModel.save();
    }

    Column {
        anchors {
            top: parent.top
            topMargin: 25
            horizontalCenter: parent.horizontalCenter
        }

        Repeater {
            model: configDialog.currentContainmentActionsModel
            delegate: RowLayout {
                width: root.width
                QtControls.Button {
                    text: model.action
                }
                QtControls.ComboBox {
                    Layout.fillWidth: true
                    model: configDialog.containmentActionConfigModel
                    textRole: "name"
                }
                QtControls.Button {
                    iconName: "configure"
                    width: height
                    onClicked: {
                        configDialog.currentContainmentActionsModel.showConfiguration(index);
                    }
                }
                QtControls.Button {
                    iconName: "dialog-information"
                    width: height
                    onClicked: {
                        configDialog.currentContainmentActionsModel.showAbout(index);
                    }
                }
                QtControls.Button {
                    iconName: "list-remove"
                    width: height
                    onClicked: {
                        configDialog.currentContainmentActionsModel.remove(index);
                    }
                }
            }
        }
        QtControls.Button {
            id: mouseInputButton
            text: i18n("Add Action")
            checkable: true
            onCheckedChanged: {
                if (checked) {
                    text = i18n("Input Here");
                    mouseInputArea.enabled = true;
                }
            }
            MouseArea {
                id: mouseInputArea
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
                enabled: false

                onClicked: {
                    if (configDialog.currentContainmentActionsModel.append(configDialog.currentContainmentActionsModel.mouseEventString(mouse.button, mouse.modifiers), "org.kde.contextmenu")) {
                        mouseInputButton.text = i18n("Add Action");
                        mouseInputButton.checked = false;
                        enabled = false;
                    }
                }

                onWheel: {
                    if (configDialog.currentContainmentActionsModel.append(configDialog.currentContainmentActionsModel.wheelEventString(wheel.pixelDelta, wheel.buttons, wheel.modifiers), "org.kde.contextmenu")) {
                        mouseInputButton.text = i18n("Add Action");
                        mouseInputButton.checked = false;
                        enabled = false;
                    }
                }
            }
        }
    }
            
}
