/*
 *  SPDX-FileCopyrightText: 2021 Devin Lin <espidev@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiComponents
import org.kde.plasma.workspace.dialogs as PWD

Kirigami.AbstractApplicationWindow {
    id: root

    width: 600
    height: 600

    PWD.SystemDialog {
        id: simple
        mainText: "Reset Data"
        subtitle: "This will reset all of your data."
        iconName: "documentinfo"

        standardButtons: T.DialogButtonBox.Ok | T.DialogButtonBox.Cancel
    }

    PWD.SystemDialog {
        id: simpleList
        mainText: "Reset Data"
        subtitle: "This will reset all of your data."
        iconName: "documentinfo"

        ListView {
            Layout.fillWidth: true
            implicitHeight: 300

            model: ListModel {
                ListElement {
                    text: "Banana"
                }
                ListElement {
                    text: "Banana 1"
                }
                ListElement {
                    text: "Banana 2"
                }
                ListElement {
                    text: "Banana 3"
                }
            }
            delegate: QQC2.ItemDelegate {
                required text

                icon.name: "kate"
                checkable: true
                width: ListView.view?.width
            }
        }

        standardButtons: T.DialogButtonBox.Ok | T.DialogButtonBox.Cancel
    }

    PWD.SystemDialog {
        id: desktopPolkit
        mainText: "Authentication Required"
        subtitle: "Authentication is needed to run `/usr/bin/ls` as the super user."
        iconName: "im-user-online"

        Kirigami.PasswordField {}

        standardButtons: T.DialogButtonBox.Ok | T.DialogButtonBox.Cancel
        actions: [
            Kirigami.Action {
                text: "Details"
                icon.name: "documentinfo"
                onTriggered: desktopPolkit.close()
            }
        ]
    }

    PWD.SystemDialog {
        id: xdgDialog
        mainText: "Wallet access"
        subtitle: "Share your wallet with 'Somebody'."
        iconName: "kwallet"
        acceptable: false

        standardButtons: T.DialogButtonBox.Ok | T.DialogButtonBox.Cancel
        Component.onCompleted: {
            dialogButtonBox.standardButton(T.DialogButtonBox.Ok).text = "Share"
        }
        actions: [
            Kirigami.Action {
                text: "Something Happens"
                icon.name: "documentinfo"
                onTriggered: xdgDialog.acceptable = true
            }
        ]
    }

    PWD.SystemDialog {
        id: appchooser
        title: "Open with…"
        iconName: "applications-all"

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.CheckBox {
                Layout.fillWidth: true
                text: i18nc("@option:check %1 is description of a file type, like 'PNG image'", "Always open %1 files with the chosen app", "PNG")
            }

            QQC2.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                maximumLineCount: 3

                text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris at viverra mi."
                wrapMode: Text.WordWrap

                onLinkActivated: {
                    AppChooserData.openDiscover()
                }
            }

            QQC2.Frame {
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

                QQC2.ScrollView {
                    id: scrollView

                    anchors.fill: parent
                    implicitHeight: grid.cellHeight * 3

                    GridView {
                        id: grid

                        readonly property int gridDelegateSize: Kirigami.Units.iconSizes.huge + (Kirigami.Units.gridUnit * 4)

                        currentIndex: -1 // Don't pre-select anything as that doesn't make sense here

                        cellWidth: {
                            const columns = Math.max(Math.floor(scrollView.availableWidth / gridDelegateSize), 2);
                            return Math.floor(scrollView.availableWidth / columns) - 1;
                        }
                        cellHeight: gridDelegateSize

                        model: ListModel {
                            ListElement {
                                applicationName: "banana"
                            }
                            ListElement {
                                applicationName: "banana1"
                            }
                            ListElement {
                                applicationName: "banana2"
                            }
                            ListElement {
                                applicationName: "banana3"
                            }
                        }
                        delegate: Item {
                            id: delegate

                            required property var model

                            height: grid.cellHeight
                            width: grid.cellWidth

                            function activate() {}

                            HoverHandler {
                                id: hoverhandler
                            }

                            TapHandler {
                                id: taphandler
                                onTapped: delegate.activate()
                            }

                            Rectangle {
                                anchors.fill: parent
                                visible: hoverhandler.hovered || delegate.GridView.isCurrentItem
                                border.color: Kirigami.Theme.highlightColor
                                border.width: 1
                                color: taphandler.pressed ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.highlightColor, 0.3)
                                radius: Kirigami.Units.smallSpacing
                            }

                            ColumnLayout {
                                anchors {
                                    top: parent.top
                                    left: parent.left
                                    right: parent.right
                                    margins: Kirigami.Units.largeSpacing
                                }
                                spacing: 0 // Items add their own as needed here

                                Kirigami.Icon {
                                    Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                                    Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                                    Layout.bottomMargin: Kirigami.Units.largeSpacing
                                    Layout.alignment: Qt.AlignHCenter
                                    source: "kalgebra"
                                    smooth: true
                                }

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop
                                    horizontalAlignment: Text.AlignHCenter
                                    text: delegate.model.applicationName
                                    // font.bold: delegate.model.applicationDesktopFile === AppChooserData.defaultApp
                                    elide: Text.ElideRight
                                    maximumLineCount: 2
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }

            QQC2.Button {
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

    PWD.SystemDialog {
        id: mobilePolkit
        mainText: "Authentication Required"
        subtitle: "Authentication is needed to run `/usr/bin/ls` as the super user."

        ColumnLayout {
            width: Kirigami.Units.gridUnit * 20

            KirigamiComponents.Avatar {
                implicitHeight: Kirigami.Units.iconSizes.medium
                implicitWidth: Kirigami.Units.iconSizes.medium
                Layout.alignment: Qt.AlignHCenter
            }
            Kirigami.PasswordField {
                Layout.fillWidth: true
            }
        }

        standardButtons: T.DialogButtonBox.Ok | T.DialogButtonBox.Cancel
        actions: [
            QQC2.Action {
                text: "Details"
                icon.name: "documentinfo"
                onTriggered: mobilePolkit.close()
            }
        ]
    }

    PWD.SystemDialog {
        id: sim
        mainText: "SIM Locked"
        subtitle: "Please enter your SIM PIN in order to unlock it."

        width: Kirigami.Units.gridUnit * 20
        standardButtons: T.DialogButtonBox.Ok | T.DialogButtonBox.Cancel

        Kirigami.PasswordField {
            Layout.fillWidth: true
        }
    }

    PWD.SystemDialog {
        id: device
        mainText: "Device Request"
        subtitle: "Allow <b>PureMaps</b> to access your location?"

        layout: Qt.Vertical

        actions: [
            QQC2.Action {
                text: "Allow all the time"
                onTriggered: device.accept()
            },
            QQC2.Action {
                text: "Allow only while the app is in use"
                onTriggered: device.accept()
            },
            QQC2.Action {
                text: "Deny"
                onTriggered: device.accept()
            }
        ]
    }

    PWD.SystemDialog {
        id: wifi
        mainText: "eduroam"

        Kirigami.FormLayout {
            QQC2.ComboBox {
                model: ["PEAP"]
                Layout.fillWidth: true
                Kirigami.FormData.label: "EAP method:"
                currentIndex: 0
            }
            QQC2.ComboBox {
                model: ["MSCHAPV2"]
                Layout.fillWidth: true
                Kirigami.FormData.label: "Phase 2 authentication:"
                currentIndex: 0
            }
            QQC2.TextField {
                Kirigami.FormData.label: "Domain:"
                Layout.fillWidth: true
                text: ""
            }
            QQC2.TextField {
                Kirigami.FormData.label: "Identity:"
                Layout.fillWidth: true
            }
            QQC2.TextField {
                Kirigami.FormData.label: "Username:"
                Layout.fillWidth: true
            }
            Kirigami.PasswordField {
                Kirigami.FormData.label: "Password:"
                Layout.fillWidth: true
            }
        }

        standardButtons: T.DialogButtonBox.Ok | T.DialogButtonBox.Cancel

        Component.onCompleted: {
            dialogButtonBox.standardButton(T.DialogButtonBox.Ok).text = "Save"
        }
    }

    component DialogDelegate : QQC2.ItemDelegate {
        required property PWD.SystemDialog dialog

        Layout.fillWidth: true

        onClicked: dialog.present()
    }

    QQC2.ScrollView {
        id: mainScrollView
        anchors.fill: parent

        Column {
            width: mainScrollView.availableWidth

            Repeater {
                model: dialogsModel

                QQC2.ItemDelegate {
                    required property PWD.SystemDialog dialog
                    required text

                    width: parent?.width
                    height: Kirigami.Units.gridUnit * 3

                    onClicked: dialog.present()
                }
            }
        }
    }

    ListModel {
        id: dialogsModel
        Component.onCompleted: {
            const data = [
                {
                    text: "Simple dialog (Desktop)",
                    dialog: simple,
                },
                {
                    text: "Simple List",
                    dialog: simpleList,
                },
                {
                    text: "Polkit dialog (Desktop)",
                    dialog: desktopPolkit,
                },
                {
                    text: "App Chooser(-ish)",
                    dialog: appchooser,
                },
                {
                    text: "XDG dialog (Desktop)",
                    dialog: xdgDialog,
                },
                {
                    text: "Polkit dialog (Mobile)",
                    dialog: mobilePolkit,
                },
                {
                    text: "SIM PIN dialog (Mobile)",
                    dialog: sim,
                },
                {
                    text: "Device request dialog (Mobile)",
                    dialog: device,
                },
                {
                    text: "Wi-Fi Dialog (Mobile)",
                    dialog: wifi,
                },
            ];
            data.forEach(item => append(item));
        }
    }
}
