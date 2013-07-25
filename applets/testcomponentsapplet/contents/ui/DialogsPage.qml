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
import QtQuick.Window 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.qtextracomponents 2.0 as QtExtras

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
                id: radio
                checkable: true
                iconSource: "dialog-ok"
                text: "Window"
            }
            Window {
                title: radio.text
                id: qWindow
                visible: radio.checked
                width: childrenRect.width
                height: childrenRect.height
                color: Qt.rgba(0,0,0,0)
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
        Row {
            height: _h
            spacing: _s
            PlasmaComponents.Button {
                text: "Core.Dialog"
                iconSource: "dialog-ok-apply"
                checkable: true
                //onCheckedChanged: pcDialog.visible = checked
                onCheckedChanged: pcDialog.visible = checked
            }
            PlasmaComponents.Label {
                text: pcDialog.visible ? "shown" : "hidden"
            }

            PlasmaCore.Dialog {
                id: pcDialog
                //windowFlags: Qt.Popup
                visualParent: dialogsPage
                //mainItem: dContent2
                color: Qt.rgba(0,0,0,0)

                mainItem: DialogContent {
                    id: dContent2
                    onCloseMe: pcDialog.visible = false
                }
            }
        }
        Row {
            height: _h
            spacing: _s
            PlasmaComponents.Button {
                text: "Dialog"
                iconSource: "dialog-ok-apply"
                checkable: true
                onCheckedChanged: {
                    if (checked) {
                        pcompDialog.open();
                    } else {
                        pcompDialog.close();
                    }
                }
            }
            PlasmaComponents.Label {
                text: pcompDialog.visible ? "shown" : "hidden"
            }

            PlasmaComponents.Dialog {
                id: pcompDialog
                //windowFlags: Qt.Popup
                visualParent: root
                content: DialogContent {
                    id: dContent3
                    onCloseMe: pcompDialog.close()
                }
                buttons: PlasmaComponents.ButtonRow {
                    PlasmaComponents.Button {
                        text: "Close";
                        onClicked: {
                            print("Closing...");
                            pcompDialog.close()
                        }
                    }
                    PlasmaComponents.Button {
                        text: "Accept";
                        onClicked: {
                            print("Accepting...");
                            pcompDialog.accept();
                            pcompDialog.close();
                        }
                    }
                }
            }
        }
        Row {
            height: _h
            spacing: _s
            PlasmaComponents.Button {
                text: "QueryDialog"
                iconSource: "dialog-ok-apply"
                checkable: true
                onCheckedChanged: {
                    if (checked) {
                        queryDialog.open();
                    } else {
                        queryDialog.close();
                    }
                }
            }
            PlasmaComponents.Label {
                text: queryDialog.visible ? "shown" : "hidden"
            }

            PlasmaComponents.QueryDialog {
                id: queryDialog
                //windowFlags: Qt.Popup
                visualParent: root
                titleText: "Fruit Inquiry"
                message: "Would you rather have apples or oranges?"
                acceptButtonText: "Apples"
                rejectButtonText: "Oranges"
                onButtonClicked: {
                    print("hey");
                    queryDialog.close();
                }
            }
        }
        PlasmaComponents.ButtonRow {
            id: buttonRow
            spacing: _s/2
            PlasmaComponents.Button {
                width: _h
                text: "Top"
                onClicked: {
                    locationDialog.location = PlasmaCore.Types.TopEdge;
                    locationDialog.visible = !locationDialog.visible
                }
            }
            PlasmaComponents.Button {
                text: "Bottom"
                width: _h
                onClicked: {
                    locationDialog.location = PlasmaCore.Types.BottomEdge;
                    locationDialog.visible = !locationDialog.visible
                }
            }
            PlasmaComponents.Button {
                text: "Left"
                width: _h
                onClicked: {
                    locationDialog.location = PlasmaCore.Types.LeftEdge;
                    locationDialog.visible = !locationDialog.visible
                }
            }
            PlasmaComponents.Button {
                text: "Right"
                width: _h
                onClicked: {
                    locationDialog.location = PlasmaCore.Types.RightEdge;
                    locationDialog.visible = !locationDialog.visible
                }
            }
            PlasmaComponents.Button {
                text: "Desktop"
                width: _h
                onClicked: {
                    locationDialog.location = PlasmaCore.Types.Desktop;
                    locationDialog.visible = !locationDialog.visible
                }
            }
            PlasmaComponents.Button {
                text: "Floating"
                width: _h
                onClicked: {
                    locationDialog.location = PlasmaCore.Types.Floating;
                    locationDialog.visible = !locationDialog.visible
                }
            }
            PlasmaComponents.Button {
                text: "FullScreen"
                width: _h
                onClicked: {
                    locationDialog.location = PlasmaCore.Types.FullScreen;
                    locationDialog.visible = !locationDialog.visible
                }
            }
        }
        PlasmaCore.Dialog {
            id: locationDialog
            visualParent: buttonRow
            mainItem: DialogContent {
                id: dContent4
                onCloseMe: locationDialog.visible = false
            }
        }
    }
}

