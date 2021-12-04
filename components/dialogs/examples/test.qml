/*
 *  SPDX-FileCopyrightText: 2021 Devin Lin <espidev@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Dialogs 1.2 as Dialogs
import QtGraphicalEffects 1.12
import org.kde.kirigami 2.19 as Kirigami
import org.kde.plasma.workspace.dialogs 1.0

Kirigami.AbstractApplicationWindow {
    id: root
    
    width: 600
    height: 600

    SystemDialog {
        id: simple
        mainText: "Reset Data"
        subtitle: "This will reset all of your data."
        iconName: "documentinfo"

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    SystemDialog {
        id: simpleList
        mainText: "Reset Data"
        subtitle: "This will reset all of your data."
        iconName: "documentinfo"

        ListView {
            Layout.fillWidth: true
            implicitHeight: 300

            model: ListModel {
                ListElement {
                    display: "banana"
                }
                ListElement {
                    display: "banana1"
                }
                ListElement {
                    display: "banana2"
                }
                ListElement {
                    display: "banana3"
                }
            }
            delegate: Kirigami.BasicListItem {
                icon: "kate"
                label: display
                highlighted: false
                checkable: true
            }
        }

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    SystemDialog {
        id: desktopPolkit
        mainText: "Authentication Required"
        subtitle: "Authentication is needed to run `/usr/bin/ls` as the super user."
        iconName: "im-user-online"

        Kirigami.PasswordField {}

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        actions: [
            Kirigami.Action {
                text: "Details"
                iconName: "documentinfo"
                onTriggered: desktopPolkit.close()
            }
        ]
    }

    SystemDialog {
        id: xdgDialog
        mainText: "Wallet access"
        subtitle: "Share your wallet with 'Somebody'."
        iconName: "kwallet"
        acceptable: false

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        Component.onCompleted: {
            dialogButtonBox.standardButton(DialogButtonBox.Ok).text = "Share"
        }
        actions: [
            Kirigami.Action {
                text: "Something Happens"
                iconName: "documentinfo"
                onTriggered: xdgDialog.acceptable = true
            }
        ]
    }

    SystemDialog {
        id: appchooser
        title: "Open with..."
        iconName: "applications-all"
        ColumnLayout {
            Text {
                text: "height: " + parent.height + " / " + xdgDialog.height
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                maximumLineCount: 3

                text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris at viverra mi. Maecenas volutpat et nisi ac scelerisque. Mauris pulvinar blandit dapibus. Nulla facilisi. Donec congue imperdiet maximus. Aliquam gravida velit sed mattis convallis. Nam id nisi egestas nibh ultrices varius quis at sapien."
                wrapMode: Text.WordWrap

                onLinkActivated: {
                    AppChooserData.openDiscover()
                }
            }

            Frame {
                id: viewBackground
                Layout.fillWidth: true
                Layout.fillHeight: true
                Kirigami.Theme.inherit: false
                Kirigami.Theme.colorSet: Kirigami.Theme.View
                background: Rectangle {
                    color: Kirigami.Theme.backgroundColor
                    property color borderColor: Kirigami.Theme.textColor
                    border.color: Qt.rgba(borderColor.r, borderColor.g, borderColor.b, 0.3)
                }

                ScrollView {
                    anchors.fill: parent
                    implicitHeight: grid.cellHeight * 3

                    GridView {
                        id: grid

                        cellHeight: Kirigami.Units.iconSizes.huge + 50
                        cellWidth: Kirigami.Units.iconSizes.huge + 80

                        model: ListModel {
                            ListElement {
                                display: "banana"
                            }
                            ListElement {
                                display: "banana1"
                            }
                            ListElement {
                                display: "banana2"
                            }
                            ListElement {
                                display: "banana3"
                            }
                        }
                        delegate: Rectangle {
                            color: "blue"
                            height: grid.cellHeight
                            width: grid.cellWidth

                            Kirigami.Icon {
                                source: "kalgebra"
                            }
                        }
                    }
                }
            }

            Button {
                id: showAllAppsButton
                Layout.alignment: Qt.AlignHCenter
                icon.name: "view-more-symbolic"
                text: "Show More"

                onClicked: {
                    visible = false
                }
            }

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                visible: !showAllAppsButton.visible
                opacity: visible
            }
        }
    }

    SystemDialog {
        id: mobilePolkit
        mainText: "Authentication Required"
        subtitle: "Authentication is needed to run `/usr/bin/ls` as the super user."

        ColumnLayout {
            width: Kirigami.Units.gridUnit * 20

            Kirigami.Avatar {
                implicitHeight: Kirigami.Units.iconSizes.medium
                implicitWidth: Kirigami.Units.iconSizes.medium
                Layout.alignment: Qt.AlignHCenter
            }
            Kirigami.PasswordField {
                Layout.fillWidth: true
            }
        }

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        actions: [
            Kirigami.Action {
                text: "Details"
                iconName: "documentinfo"
                onTriggered: mobilePolkit.close()
            }
        ]
    }

    SystemDialog {
        id: sim
        mainText: "SIM Locked"
        subtitle: "Please enter your SIM PIN in order to unlock it."

        width: Kirigami.Units.gridUnit * 20
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        Kirigami.PasswordField {
            Layout.fillWidth: true
        }
    }

    SystemDialog {
        id: device
        mainText: "Device Request"
        subtitle: "Allow <b>PureMaps</b> to access your location?"

        layout: Qt.Vertical

        actions: [
            Kirigami.Action {
                text: "Allow all the time"
                onTriggered: device.accept()
            },
            Kirigami.Action {
                text: "Allow only while the app is in use"
                onTriggered: device.accept()
            },
            Kirigami.Action {
                text: "Deny"
                onTriggered: device.accept()
            }
        ]
    }

    SystemDialog {
        id: wifi
        mainText: "eduroam"

        Kirigami.FormLayout {
            ComboBox {
                model: ["PEAP"]
                Layout.fillWidth: true
                Kirigami.FormData.label: "EAP method:"
                currentIndex: 0
            }
            ComboBox {
                model: ["MSCHAPV2"]
                Layout.fillWidth: true
                Kirigami.FormData.label: "Phase 2 authentication:"
                currentIndex: 0
            }
            TextField {
                Kirigami.FormData.label: "Domain:"
                Layout.fillWidth: true
                text: ""
            }
            TextField {
                Kirigami.FormData.label: "Identity:"
                Layout.fillWidth: true
            }
            TextField {
                Kirigami.FormData.label: "Username:"
                Layout.fillWidth: true
            }
            Kirigami.PasswordField {
                Kirigami.FormData.label: "Password:"
                Layout.fillWidth: true
            }
        }

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        Component.onCompleted: {
            dialogButtonBox.standardButton(DialogButtonBox.Ok).text = "Save"
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        Button {
            text: "Simple dialog (Desktop)"
            onClicked: {
                simple.present()
            }
        }
        Button {
            text: "Simple List"
            onClicked: {
                simpleList.present()
            }
        }
        Button {
            text: "Polkit dialog (Desktop)"
            onClicked: {
                desktopPolkit.present()
            }
        }
        Button {
            text: "App Chooser(-ish)"
            onClicked: {
                appchooser.present()
            }
        }
        Button {
            text: "XDG dialog (Desktop)"
            onClicked: {
                xdgDialog.present()
            }
        }
        Button {
            text: "Polkit dialog (Mobile)"
            onClicked: {
                mobilePolkit.present()
            }
        }
        Button {
            text: "SIM PIN dialog (Mobile)"
            onClicked: {
                sim.present()
            }
        }
        Button {
            text: "Device request dialog (Mobile)"
            onClicked: {
                device.present()
            }
        }
        Button {
            text: "Wifi Dialog (Mobile)"
            onClicked: {
                wifi.present()
            }
        }
    }
} 

