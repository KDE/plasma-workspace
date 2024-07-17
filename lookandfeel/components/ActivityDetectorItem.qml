/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick

Item {
    id: root
    readonly property bool active: pointHandler.active || keyPressed || anyActivity

    readonly property alias pointHandler: pointHandler
    readonly property alias hoverHandler: hoverHandler

    // True when any key is pressed.
    // Set by Keys signal handlers.
    property bool keyPressed: false
    // True when any kind of pointer or key event happens.
    // This is controlled by a timer because there would otherwise be no good
    // way to make this false again when handling release events.
    property bool anyActivity: false

    function activityDetected() {
        root.anyActivity = true;
        activityTimer.restart();
    }

    Timer {
        id: activityTimer
        running: false
        interval: 100
        onTriggered: root.anyActivity = false
    }

    property alias cursorShape: hoverHandler.cursorShape
    property point lastScenePosition: Qt.point(-1, -1)

    PointHandler {
        id: pointHandler
        acceptedDevices: PointerDevice.AllDevices
        acceptedPointerTypes: PointerDevice.AllPointerTypes
        acceptedButtons: Qt.AllButtons
        acceptedModifiers: Qt.KeyboardModifierMask
        onActiveChanged: root.activityDetected()
    }
    HoverHandler {
        id: hoverHandler
        blocking: false
        acceptedDevices: PointerDevice.AllDevices
        acceptedPointerTypes: PointerDevice.AllPointerTypes
        acceptedButtons: Qt.AllButtons
        acceptedModifiers: Qt.KeyboardModifierMask
        onPointChanged: if (point.scenePosition !== root.lastScenePosition) {
            root.lastScenePosition.x = point.scenePosition.x
            root.lastScenePosition.y = point.scenePosition.y
            root.activityDetected()
        }
    }

    // You need to set up Keys.forwardTo in order for these to work
    // or give this active focus, but that would be kind of pointless.
    Keys.onPressed: event => {
        root.keyPressed = true;
        root.activityDetected();
    }
    Keys.onReleased: event => {
        root.keyPressed = false;
        root.activityDetected();
    }
}
