/*
    SPDX-FileCopyrightText: 2017 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.VirtualKeyboard as QtVK

import org.kde.kirigami as Kirigami

QtVK.InputPanel {
    id: inputPanel
    property bool activated: false
    active: activated && InputMethod.visible
    width: parent.width

    states: [
        State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges {
                inputPanel.y: inputPanel.parent.height - inputPanel.height
                inputPanel.opacity: 1
                inputPanel.visible: true
            }
        },
        State {
            name: "hidden"
            when: !inputPanel.active
            PropertyChanges {
                inputPanel.y: inputPanel.parent.height
                inputPanel.opacity: 0
                inputPanel.visible:false
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
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutQuad
                }
                OpacityAnimator {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutQuad
                }
            }
        },
        Transition {
            to: "hidden"
            ParallelAnimation {
                YAnimator {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InQuad
                }
                OpacityAnimator {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InQuad
                }
            }
        }
    ]
}
