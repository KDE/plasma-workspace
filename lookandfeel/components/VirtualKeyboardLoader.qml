/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami

Loader {
    id: loader

    required property T.TextField passwordField

    readonly property bool keyboardActive: item?.active ?? false

    function showHide() {
        state = state === "hidden" ? "visible" : "hidden";
    }

    source: Qt.platform.pluginName.includes("wayland")
        ? Qt.resolvedUrl("./VirtualKeyboard_wayland.qml")
        : Qt.resolvedUrl("./VirtualKeyboard.qml")

    onKeyboardActiveChanged: {
        if (keyboardActive) {
            state = "visible";
            // Otherwise the password field loses focus and virtual keyboard
            // keystrokes get eaten
            passwordField.forceActiveFocus();
        } else {
            state = "hidden";
        }
    }

    // Usually, anchors on a top-level component is a bad idea, but this is a
    // tightly integrated component shared only between lock screen and SDDM
    anchors {
        left: parent.left
        right: parent.right
    }

    state: "hidden"

    states: [
        State {
            name: "visible"
            PropertyChanges {
                target: loader
                y: Window.height - loader.height
            }
        },
        State {
            name: "hidden"
            PropertyChanges {
                target: loader
                y: Window.height * 0.75
            }
        }
    ]

    transitions: [
        Transition {
            from: "hidden"
            to: "visible"
            SequentialAnimation {
                ScriptAction {
                    script: {
                        loader.item.activated = true;
                        Qt.inputMethod.show();
                    }
                }
                NumberAnimation {
                    property: "y"
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutQuad
                }
            }
        },
        Transition {
            from: "visible"
            to: "hidden"
            SequentialAnimation {
                NumberAnimation {
                    property: "y"
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InQuad
                }
                ScriptAction {
                    script: {
                        loader.item.activated = false;
                        Qt.inputMethod.hide();
                    }
                }
            }
        }
    ]
}
