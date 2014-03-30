/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: taskRoot
    objectName: "systemtrayplasmoid"

//     width: 48
//     height: 48
    property bool vertical: (plasmoid.formFactor == PlasmaCore.Types.Vertical)

    Layout.fillWidth: !vertical
    Layout.fillHeight: vertical

    property int dim: 128

    property int preferredWidth: dim * 2
    property int preferredHeight: dim

    property int implicitWidth: dim * 2
    property int implicitHeight: dim

    Layout.minimumWidth: dim * 2
    Layout.minimumHeight: dim

    //
    //Rectangle { anchors.fill: parent; color: "orange"; opacity: 0.8; }

    /*
    Plasmoid.compactRepresentation: Component {
        Rectangle { color: "orange"; opacity: 1; }

    }
    */
    Plasmoid.compactRepresentation: Component {
        Item {
            Rectangle {
                Layout.fillWidth: !vertical
                Layout.fillHeight: vertical

    //             property int preferredWidth: 100
    //             property int preferredHeight: 22

    //             property int implicitWidth: 100
    //             property int implicitHeight: 256
    //             anchors.fill: parent;
                border.width: 2;
                border.color: "black";
                color: "green";
                opacity: 0.0;

            }
            PlasmaCore.IconItem {
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                source: "mail-unread"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: plasmoid.expanded = !plasmoid.expanded
            }
        }
    }

    Rectangle {
        border.width: 2;
        border.color: "black";
        color: "blue";
        opacity: 0.8;
        visible: false
        anchors.fill: parent;
    }

    PlasmaCore.IconItem {
        source: "konqueror"
        anchors.fill: parent;
        anchors.margins: 12
    }

    Component.onCompleted: {
        print("Loaded plasmoid's main.qml.");
    }
//     MouseArea {
//         anchors.fill: parent
//         onClicked: {
//             print(taskRoot.objectName + " has been clicked on.");
//             print(" Is there a plasmoid? " + plasmoid != undefined);
//             print(" Plasmoid ID: " + plasmoid.id);
//         }
//     }
}

