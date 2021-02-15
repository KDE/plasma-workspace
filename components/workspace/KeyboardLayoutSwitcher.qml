/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 * SPDX-FileCopyrightText: 2020 Andrey Butirsky <butirsky@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.12
import org.kde.plasma.workspace.keyboardlayout 1.0

MouseArea {
    property alias keyboardLayout: keyboardLayout
    readonly property bool hasMultipleKeyboardLayouts: keyboardLayout.layoutsList.length > 1
    readonly property var layoutNames: keyboardLayout.layoutsList.length ? keyboardLayout.layoutsList[keyboardLayout.layout]
                                                                         : { shortName: "", displayName: "", longName: "" }

    onClicked: keyboardLayout.switchToNextLayout()

    property int wheelDelta: 0

    onWheel: {
        // Magic number 120 for common "one click"
        // See: https://qt-project.org/doc/qt-5/qml-qtquick-wheelevent.html#angleDelta-prop
        var delta = wheel.angleDelta.y || wheel.angleDelta.x;
        wheelDelta += delta;
        while (wheelDelta >= 120) {
            wheelDelta -= 120;
            keyboardLayout.switchToPreviousLayout();
        }
        while (wheelDelta <= -120) {
            wheelDelta += 120;
            keyboardLayout.switchToNextLayout();
        }
    }

    KeyboardLayout {
        id: keyboardLayout
    }
}
