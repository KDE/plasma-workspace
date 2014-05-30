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

PlasmaCore.FrameSvgItem {
    id: shutdownUi
    property var buttonForAccel: Array()

//     width: realMarginLeft + 2 * buttonsLayout.width + realMarginRight
//     height: realMarginTop + units.smallSpacing + automaticallyDoLabel.height + units.smallSpacing + buttonsLayout.height + realMarginBottom
    width: Math.max(leftPicture.width * 2, shutdownButton.implicitWidth*2)
    height: Math.max(leftPicture.height, buttonsLayout.height)

    imagePath: "dialogs/shutdowndialog"

    signal logoutRequested()
    signal haltRequested()
    signal suspendRequested(int spdMethod)
    signal rebootRequested()
    signal rebootRequested2(int opt)
    signal cancelRequested()
    signal lockScreenRequested()

    property variant focusedButton: null
    property variant lastButton: null
    property int automaticallyDoSeconds: 30
    onLastButtonChanged: automaticallyDoSeconds = 30
    onAutomaticallyDoSecondsChanged: {
        if (focusedButton && automaticallyDoSeconds == 0) {
            focusedButton.clicked()
        }
    }

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
        switch (sdtype) {
            case ShutdownType.ShutdownTypeNone:
                focusedButton = logoutButton
                break;
            case ShutdownType.ShutdownTypeHalt:
                if (maysd)
                    focusedButton = shutdownButton;
                break;
            case ShutdownType.ShutdownTypeReboot:
                if (maysd)
                    focusedButton = rebootButton
                break;
        }

        // implement label accelerators in the buttons (the '&' in button's text).
        //TODO: dont' add shutdownButton and rebootButton if not "maysd"
        var buttons = [ logoutButton, shutdownButton, rebootButton, cancelButton ]
        for (var b in buttons ) {
            if (buttons[b].accelKey > -1) {
                shutdownUi.buttonForAccel[String.fromCharCode(buttons[b].accelKey)] = buttons[b];
            }
        }
    }

    // trigger action on Alt+'accelerator' key press. For example: if KSMButton.text == &Cancel,
    // pressing Alt+'C' or Alt+'c' will trigger KSMButton.clicked().
    Keys.onPressed: {
        if ((event.modifiers & Qt.AltModifier) && shutdownUi.buttonForAccel[String.fromCharCode(event.key)] != undefined) {
            shutdownUi.buttonForAccel[String.fromCharCode(event.key)].clicked()
        }
    }

    Timer {
        repeat: true
        running: focusedButton!=null
        interval: 1000

        onTriggered: {
            --automaticallyDoSeconds
        }
    }

    PlasmaComponents.Label {
        id: automaticallyDoLabel
        anchors {
            top: parent.top
            right: parent.right
            left: parent.left
            margins: units.smallSpacing
        }

        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignRight
        text: if (focusedButton.text == logoutButton.text) {
                i18np("Logging out in 1 second.", "Logging out in %1 seconds.", automaticallyDoSeconds)
            } else if (focusedButton.text == shutdownButton.text) {
                i18np("Turning off computer in 1 second.", "Turning off computer in %1 seconds.", automaticallyDoSeconds)
            } else if (focusedButton.text == rebootButton.text) {
                i18np("Restarting computer in 1 second.", "Restarting computer in %1 seconds.", automaticallyDoSeconds)
            } else {
                ""
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
            margins: units.largeSpacing
        }

        svg: PlasmaCore.Svg {
            imagePath: "dialogs/shutdowndialog"
        }
        elementId: "picture"
    }

    FocusScope {
        anchors.margins: units.largeSpacing

        anchors {
            top: automaticallyDoLabel.bottom
            right: parent.right
            bottom: parent.bottom
            left: leftPicture.right
            margins: units.largeSpacing
        }

        Column {
            id: buttonsLayout
            spacing: units.smallSpacing
            width: parent.width

            PlasmaComponents.Button {
                id: logoutButton
                text: i18n("Logout")
                iconSource: "system-log-out"
                width: parent.width
                visible: (choose || sdtype == ShutdownType.ShutdownTypeNone)

                onClicked: {
                    //console.log("main.qml: logoutRequested")
                    logoutRequested()
                }

                onActiveFocusChanged: if (activeFocus) {
                    shutdownUi.focusedButton = logoutButton
                }
            }

            PlasmaComponents.Button {
                id: shutdownButton
                text: i18n("Turn Off Computer")
                iconSource: "system-shutdown"
                width: parent.width
                visible: (choose || sdtype == ShutdownType.ShutdownTypeHalt)
//                     menu: spdMethods.StandbyState | spdMethods.SuspendState | spdMethods.HibernateState

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

                onActiveFocusChanged: if (activeFocus) {
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
                width: parent.width
                visible: (choose || sdtype == ShutdownType.ShutdownTypeReboot)
//                     menu: rebootOptions["options"].length > 0

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
//
//                         }
//                         contextMenu.open()
//                     }

                onActiveFocusChanged: if (activeFocus) {
                    shutdownUi.focusedButton = rebootButton
                }
            }

            Item { width: 1; height: 1 } //add double spacing for the cancel button

            PlasmaComponents.Button {
                id: cancelButton

                text: i18n("Cancel")
                iconSource: "dialog-cancel"
                width: parent.width

                onClicked: {
                    cancelRequested()
                }

                onActiveFocusChanged: if (activeFocus) {
                    shutdownUi.focusedButton = cancelButton
                }
            }

            Component {
                id: rebootOptionsComponent
                ContextMenu {
                    visualParent: rebootButton
                }
            }
        }
    }
}
