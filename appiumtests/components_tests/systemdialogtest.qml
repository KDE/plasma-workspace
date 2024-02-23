/*
 *  SPDX-FileCopyrightText: 2021 Devin Lin <espidev@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
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

        standardButtons: QQC2.DialogButtonBox.Yes
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
            delegate: QQC2.ItemDelegate {
                icon.name: "kate"
                text: model.display
                checkable: true
            }
        }

        standardButtons: QQC2.DialogButtonBox.Cancel
    }

    PWD.SystemDialog {
        id: desktopPolkit
        mainText: "Authentication Required"
        subtitle: "Authentication is needed to run `/usr/bin/ls` as the super user."
        iconName: "im-user-online"
        acceptable: false

        Kirigami.PasswordField {}

        standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel
        actions: [
            QQC2.Action {
                text: "Details"
                icon.name: "documentinfo"
                onTriggered: desktopPolkit.close()
            }
        ]
    }

    ColumnLayout {
        anchors.fill: parent
        QQC2.Button {
            text: "Simple dialog (Desktop)"
            onClicked: {
                simple.present()
            }
        }
        QQC2.Button {
            text: "Simple List"
            onClicked: {
                simpleList.present()
            }
        }
        QQC2.Button {
            text: "Polkit dialog (Desktop)"
            onClicked: {
                desktopPolkit.present()
            }
        }
    }
}
