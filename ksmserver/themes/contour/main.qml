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

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0

PlasmaCore.FrameSvgItem {
    id: shutdownUi
    property int iconSize: 96
    property int realMarginTop: margins.top
    property int realMarginBottom: margins.bottom
    property int realMarginLeft: margins.left
    property int realMarginRight: realMarginLeft
    width: 3*realMarginLeft + dialog.width + 3*realMarginRight
    height: 2*realMarginTop + dialog.height + 2*realMarginBottom
    property int automaticallyDoSeconds: 5

    imagePath: "dialogs/shutdowndialog"

    signal logoutRequested()
    signal haltRequested()
    signal suspendRequested(int spdMethod)
    signal rebootRequested()
    signal rebootRequested2(int opt)
    signal cancelRequested()
    signal lockScreenRequested()

    PlasmaCore.Theme {
        id: theme
    }

    Component.onCompleted: {
        if (margins.left == 0) {
            realMarginTop = 9
            realMarginBottom = 7
            realMarginLeft = 12
            realMarginRight = 12
        }

        //console.log("contour.qml: maysd("+maysd+") choose ("+choose+") ("+sdtype+")")
        //console.log("contour.qml: defualtFont.pointSize == " + theme.defaultFont.pointSize)
    }

    Timer {
        id: timer
        repeat: true
        running: true
        interval: 1000

        onTriggered: {
            if (automaticallyDoSeconds <= 0) { // timeout is at 0, do selected action
                running = false
                sleepButton.clicked(null)
            }
            automaticallyDoLabel.text = i18ndp("ksmserver", "Sleeping in 1 second",
                                               "Sleeping in %1 seconds", automaticallyDoSeconds)
            --automaticallyDoSeconds;
        }
    }

    Column {
        id: dialog
        spacing: 5

        anchors {
            centerIn: parent
        }

        Text {
            id: automaticallyDoLabel
            text: " "
            // pixelSize does not work with PlasmaComponents.Label, so I am using a Text element here.
            font.pixelSize: Math.max(12, theme.defaultFont.pointSize)
            color: theme.textColor
            anchors {
                horizontalCenter: parent.horizontalCenter
            }
        }

        Row {
            id: iconRow
            spacing: 3*realMarginLeft

            ContourButton {
                id: lockScreenButton
                iconSource: "system-lock-screen"
                iconSize: shutdownUi.iconSize
                text: i18nd("ksmserver", "Lock")
                font.pixelSize: 1.5*automaticallyDoLabel.font.pixelSize

                onClicked: {
                    //console.log("contour.qml: lock screen requested")
                    timer.running = false
                    lockScreenRequested();
                }
            }

            ContourButton {
                id: sleepButton
                iconSource: "system-suspend"
                iconSize: shutdownUi.iconSize
                text: i18n("ksmserver", "Sleep")
                font.pixelSize: 1.5*automaticallyDoLabel.font.pixelSize

                onClicked: {
                    //console.log("contour.qml: sleep requested")
                    timer.running = false
                    if (spdMethods.SuspendState) {
                        suspendRequested(2); // Solid::PowerManagement::SuspendState
                    } else if (spdMethods.StandbyState) {
                        suspendRequested(1); // Solid::PowerManagement::StandbyState
                    } else {
                        console.log("contour.qml: system does not support suspend")
                    }
                }
            }

            ContourButton {
                id: shutdownButton
                iconSource: "system-shutdown"
                iconSize: shutdownUi.iconSize
                text: i18nd("ksmserver", "Turn off")
                font.pixelSize: 1.5*automaticallyDoLabel.font.pixelSize

                onClicked: {
                    //console.log("contour.qml turn off requested")
                    timer.running = false
                    haltRequested()
                }
            }
        }
    }
}
