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

/**
 TODO:
 . use kde-runtime/plasma/declarativeimports/plasmacomponents/qml/ContextMenu.qml
 instead of a custom ContextMenu component.
 */
import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import "helper.js" as Helper
import "."
PlasmaCore.FrameSvgItem {
    id: shutdownUi
    property int realMarginTop: margins.top
    property int realMarginBottom: margins.bottom
    property int realMarginLeft: margins.left
    property int realMarginRight: margins.right

    width: realMarginLeft + 2 * buttonsLayout.width + realMarginRight
    height: realMarginTop + 4 + automaticallyDoLabel.height + 4 + buttonsLayout.height + realMarginBottom

    imagePath: "dialogs/shutdowndialog"

    signal logoutRequested()
    signal haltRequested()
    signal suspendRequested(int spdMethod)
    signal rebootRequested()
    signal rebootRequested2(int opt)
    signal cancelRequested()
    signal lockScreenRequested()

    property variant focusedButton: 0
    property variant lastButton: 0
    property int automaticallyDoSeconds: 30

//     PlasmaCore.Theme {
//         id: theme
//     }

    /* This is not necessary for themes that follow the specification.
       Uncomment this and [1] below if the dialog appears without borders or background.
       You should not use broken themes though.
      PlasmaCore.SvgItem {
        id: background

        anchors {
            top: parent.top
            topMargin: realMarginTop
            bottom: parent.bottom
            bottomMargin: realMarginBottom
            left: parent.left
            leftMargin: realMarginLeft
            right: parent.right
            rightMargin: realMarginRight
        }

        svg: PlasmaCore.Svg {
            imagePath: "dialogs/shutdowndialog"
        }
        elementId: "center"
    }*/

    Component.onCompleted: {
        // Hacky but works :-)
        logoutButton.width = buttonsLayout.width
        shutdownButton.width = buttonsLayout.width
        rebootButton.width = buttonsLayout.width

        if (margins.left == 0) {
            realMarginTop = 9
            realMarginBottom = 7
            realMarginLeft = 12
            realMarginRight = 12
        }

        if (leftPicture.naturalSize.width < 1) {
            // [1]
            //background.elementId = "background"
            shutdownUi.width = buttonsLayout.width + realMarginLeft + realMarginRight
            shutdownUi.height += realMarginTop + realMarginBottom
            automaticallyDoLabel.anchors.topMargin = 2*realMarginTop
            automaticallyDoLabel.anchors.rightMargin = 2*realMarginRight
            leftPicture.anchors.leftMargin = 2*realMarginLeft
            buttonsLayout.anchors.rightMargin = 2*realMarginRight
        } else {
            var pictureWidth = buttonsLayout.height * leftPicture.naturalSize.width / leftPicture.naturalSize.height

            if (pictureWidth < leftPicture.naturalSize.width) {
                leftPicture.width = pictureWidth;
                leftPicture.height = pictureWidth * leftPicture.naturalSize.height / leftPicture.naturalSize.width
            }

            shutdownUi.width = realMarginLeft + leftPicture.width + buttonsLayout.width + realMarginRight
        }

        if (choose || sdtype == ShutdownType.ShutdownTypeNone) {
            if (sdtype == ShutdownType.ShutdownTypeNone) {
                focusedButton = logoutButton
            }
        }

        if (maysd) {
            if(choose || sdtype == ShutdownType.ShutdownTypeHalt) {
                if (sdtype == ShutdownType.ShutdownTypeHalt) {
                    focusedButton = shutdownButton
                }
            }

            if (choose || sdtype == ShutdownType.ShutdownTypeReboot) {
                if (sdtype == ShutdownType.ShutdownTypeReboot) {
                    focusedButton = rebootButton
                }
            }
        }

//         focusedButton.forceActiveFocus()
        timer.running = true;

        // implement label accelerators in the buttons (the '&' in button's text).
        var buttons = [ logoutButton, shutdownButton, rebootButton, cancelButton ]
        for (var b = 0; b < buttons.length; ++b ) {
            if (buttons[b].accelKey > -1) {
                Helper.buttonForAccel[String.fromCharCode(buttons[b].accelKey)] = buttons[b];
            }
        }
    }

    // trigger action on Alt+'accelerator' key press. For example: if KSMButton.text == &Cancel,
    // pressing Alt+'C' or Alt+'c' will trigger KSMButton.clicked().
    Keys.onPressed: {
        if ((event.modifiers & Qt.AltModifier) && Helper.buttonForAccel[String.fromCharCode(event.key)] != undefined) {
            Helper.buttonForAccel[String.fromCharCode(event.key)].clicked()
        }
    }

    Timer {
        id: timer
        repeat: true
        running: false
        interval: 1000

        onTriggered: {
            if (focusedButton != lastButton) {
                lastButton = focusedButton
                automaticallyDoSeconds = 30
            }
            if (focusedButton != 0) {
                if (automaticallyDoSeconds <= 0) { // timeout is at 0, do selected action
                    focusedButton.clicked()
                // following code is required to provide a clean way to translate strings
                } else if (focusedButton.text == logoutButton.text) {
                    automaticallyDoLabel.text = i18np("Logging out in 1 second.",
                                                      "Logging out in %1 seconds.", automaticallyDoSeconds)
                } else if (focusedButton.text == shutdownButton.text) {
                    automaticallyDoLabel.text = i18np("Turning off computer in 1 second.",
                                                      "Turning off computer in %1 seconds.", automaticallyDoSeconds)
                } else if (focusedButton.text == rebootButton.text) {
                    automaticallyDoLabel.text = i18np("Restarting computer in 1 second.",
                                                      "Restarting computer in %1 seconds.", automaticallyDoSeconds)
                } else {
                    automaticallyDoLabel.text = ""
                }

                --automaticallyDoSeconds;
            }
        }
    }

    Text {
        id: automaticallyDoLabel
//         font.pointSize: theme.desktopFont.pointSize >= 10 ? theme.desktopFont.pointSize*0.9 : theme.desktopFont.pointSize
        color: theme.textColor
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignRight
        anchors {
            top: parent.top
            topMargin: realMarginTop
            right: parent.right
            rightMargin: realMarginRight
            left: parent.left
            leftMargin: realMarginLeft
        }
    }

    PlasmaCore.SvgItem {
        id: leftPicture
        width: naturalSize.width
        height: width * naturalSize.height / naturalSize.width
        smooth: true
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: realMarginLeft
        }

        svg: PlasmaCore.Svg {
            imagePath: "dialogs/shutdowndialog"
        }
        elementId: "picture"
    }

    FocusScope {
        id: scope
        width: buttonsLayout.width
        height: buttonsLayout.height

        anchors {
            top: automaticallyDoLabel.bottom
            topMargin: 4
            right: parent.right
            rightMargin: realMarginRight
            bottom: parent.bottom
            bottomMargin: realMarginBottom
        }

        Column {
            id: buttonsLayout
            anchors.bottom: parent.bottom
            spacing: 9

            Column {
                spacing: 4

                PlasmaComponents.Button {
                    id: logoutButton
                    text: i18n("Logout")
                    iconSource: "system-log-out"
                    anchors.right: parent.right
                    visible: (choose || sdtype == ShutdownType.ShutdownTypeNone)
//                     tabStopNext: shutdownButton
//                     tabStopBack: cancelButton

                    onClicked: {
                        //console.log("main.qml: logoutRequested")
                        logoutRequested()
                    }

                    onActiveFocusChanged: {
                        shutdownUi.focusedButton = logoutButton
                    }
                }

                PlasmaComponents.Button {
                    id: shutdownButton
                    text: i18n("Turn Off Computer")
                    iconSource: "system-shutdown"
                    anchors.right: parent.right
                    visible: (choose || sdtype == ShutdownType.ShutdownTypeHalt)
//                     menu: spdMethods.StandbyState | spdMethods.SuspendState | spdMethods.HibernateState
//                     tabStopNext: rebootButton
//                     tabStopBack: logoutButton

                    onClicked: {
                        //console.log("main.qml: haltRequested")
                        haltRequested()
                    }

//                     onPressAndHold: {
//                         if (!menu) {
//                             return
//                         }
//                         if (!contextMenu) {
//                             contextMenu = shutdownOptionsComponent.createObject(shutdownButton)
//                             if (spdMethods.StandbyState) {
//                                 // 1 == Solid::PowerManagement::StandbyState
//                                 contextMenu.append({itemIndex: 1, itemText: i18n("&Standby"), itemSubMenu: null, itemAllowAmpersand: false})
//                             }
//                             if (spdMethods.SuspendState) {
//                                 // 2 == Solid::PowerManagement::SuspendState
//                                 contextMenu.append({itemIndex: 2, itemText: i18n("Suspend to &RAM"), itemSubMenu: null, itemAllowAmpersand: false})
//                             }
//                             if (spdMethods.HibernateState) {
//                                 // 4 == Solid::PowerManagement::HibernateState
//                                 contextMenu.append({itemIndex: 4, itemText: i18n("Suspend to &Disk"), itemSubMenu: null, itemAllowAmpersand: false})
//                             }
//                             contextMenu.clicked.connect(shutdownUi.suspendRequested)
//                         }
//                         contextMenu.open()
//                     }

                    onActiveFocusChanged: {
                        shutdownUi.focusedButton = shutdownButton
                    }
                }

                Component {
                    id: shutdownOptionsComponent
                    ContextMenu {
                        visualParent: shutdownButton
                    }
                }

                PlasmaComponents.Button {
                    id: rebootButton
                    text: i18n("Restart Computer")
                    iconSource: "system-reboot"
                    anchors.right: parent.right
                    visible: (choose || sdtype == ShutdownType.ShutdownTypeReboot)
//                     menu: rebootOptions["options"].length > 0
//                     tabStopNext: cancelButton
//                     tabStopBack: shutdownButton

                    onClicked: {
                        //console.log("main.qml: rebootRequested")
                        rebootRequested()
                    }

                    // recursive, let's hope the user does not have too many menu entries.
                    function findAndCreateMenu(index, options, menus, menuId)
                    {
                        if (index.value < 0) {
                            return;
                        }

                        var sep = " >> "
                        var text = options[index.value]
                        var pos = text.lastIndexOf(sep)
                        if (pos > -1) {
                            var temp = text.substr(0, pos).trim()
                            if (temp != menuId) {
                                menuId = temp
                            }

                            if (!menus[menuId]) {
                                //console.log("creating menu for " + menuId)
                                menus[menuId] = rebootOptionsComponent.createObject(rebootButton)
                                menus[menuId].clicked.connect(shutdownUi.rebootRequested2)
                            }
                        } else {
                            menuId = ""
                        }

                        //console.log("index == " + index.value + " of " + options.length + " '" + text + "' menuId == '" + menuId + "'");

                        var itemData = new Object
                        itemData["itemIndex"] = index.value
                        itemData["itemText"] = text

                        if (index.value == rebootOptions["default"]) {
                            itemData["itemText"] += i18nc("default option in boot loader", " (default)")
                        }

                        --index.value;
                        var currentMenuId = menuId
                        findAndCreateMenu(index, options, menus, menuId)

                        itemData["itemSubMenu"] = menus[itemData["itemText"]]

                        // remove menuId string from itemText
                        text = itemData["itemText"]
                        var i = text.lastIndexOf(sep)
                        if (i > -1) {
                            text = text.substr(i+sep.length).trim()
                        }
                        itemData["itemText"] = text
                        itemData["itemAllowAmpersand"] = true

                        //console.log("appending " + itemData["itemText"] + " to menu '" + currentMenuId + "'")
                        menus[currentMenuId].append(itemData)
                    }

//                     onPressAndHold: {
//                         if (!menu) {
//                             return
//                         }
//                         if (!contextMenu) {
//                             var options = rebootOptions["options"]
//                             //console.log("bootManager == " + bootManager)
//
//                             if (bootManager === "Grub2" || bootManager === "Burg") {
//                                 // javascript passes primitive types by value, I need this one passed by reference.
//                                 function Index() { this.value = 0 }
//                                 var index = new Index()
//                                 var menus = {}
//                                 var menuId = ""
//
//                                 // starts backwards so that the top of the stack is the first menu entry.
//                                 index.value = options.length - 1
//                                 menus[menuId] = rebootOptionsComponent.createObject(rebootButton)
//                                 menus[menuId].clicked.connect(shutdownUi.rebootRequested2)
//                                 findAndCreateMenu(index, options, menus, menuId)
//                                 contextMenu = menus[menuId]
//                             } else {
//                                 contextMenu = rebootOptionsComponent.createObject(rebootButton)
//
//                                 for (var index = 0; index < options.length; ++index) {
//                                     var itemData = new Object
//                                     itemData["itemIndex"] = index
//                                     itemData["itemText"] = options[index]
//                                     itemData["itemSubMenu"] = null
//                                     itemData["itemAllowAmpersand"] = true
//                                     if (index == rebootOptions["default"]) {
//                                         itemData["itemText"] += i18nc("default option in boot loader", " (default)")
//                                     }
//                                     contextMenu.append(itemData)
//                                 }
//                                 contextMenu.clicked.connect(shutdownUi.rebootRequested2)
//                             }
//                         }
//                         contextMenu.open()
//                     }

                    onActiveFocusChanged: {
                        shutdownUi.focusedButton = rebootButton
                    }
                }

                Component {
                    id: rebootOptionsComponent
                    ContextMenu {
                        visualParent: rebootButton
                    }
                }
            }

            PlasmaComponents.Button {
                id: cancelButton
                anchors.right: parent.right
                text: i18n("Cancel")
                iconSource: "dialog-cancel"
//                 smallButton: true
//                 tabStopNext: logoutButton
//                 tabStopBack: rebootButton

                onClicked: {
                    cancelRequested()
                }

                onActiveFocusChanged: {
                    shutdownUi.focusedButton = cancelButton
                }
            }
        }
    }
}
