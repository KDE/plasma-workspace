/********************************************************************
 This file is part of the KDE project.

Copyright (C) 2017 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
import QtQuick 2.5
import QtQuick.VirtualKeyboard 2.1
import org.kde.plasma.core 2.0 as PlasmaCore

InputPanel {
    id: inputPanel
    property bool activated: false
    active: activated && Qt.inputMethod.visible
    width: parent.width

    states: [
        State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges {
                target: inputPanel
                y: inputPanel.parent.height - inputPanel.height
                opacity: 1
                visible: true
            }
        },
        State {
            name: "hidden"
            when: !inputPanel.active
            PropertyChanges {
                target: inputPanel
                y: inputPanel.parent.height
                opacity: 0
                visible:false
            }
        }
    ]

    transitions: [
        Transition {
            to: "visible"
            ParallelAnimation {
                YAnimator {
                    // NOTE this is necessary as otherwise the keyboard always starts the transition with Y as 0, due to the internal reparenting happening when becomes active
                    from: inputPanel.parent.height
                    duration: PlasmaCore.Units.longDuration
                    easing.type: Easing.OutQuad
                }
                OpacityAnimator {
                    duration: PlasmaCore.Units.longDuration
                    easing.type: Easing.OutQuad
                }
            }
        },
        Transition {
            to: "hidden"
            ParallelAnimation {
                YAnimator {
                    duration: PlasmaCore.Units.longDuration
                    easing.type: Easing.InQuad
                }
                OpacityAnimator {
                    duration: PlasmaCore.Units.longDuration
                    easing.type: Easing.InQuad
                }
            }
        }
    ]
}
