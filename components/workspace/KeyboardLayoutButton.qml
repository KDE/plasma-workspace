/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 * SPDX-FileCopyrightText: 2020 Andrey Butirsky <butirsky@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.12
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.workspace.keyboardlayout 1.0

PlasmaComponents3.ToolButton {
    property alias keyboardLayout: keyboardLayout
    readonly property bool hasMultipleKeyboardLayouts: keyboardLayout.layoutsList.length > 1
    readonly property var layoutNames: keyboardLayout.layoutsList.length ? keyboardLayout.layoutsList[keyboardLayout.layout] : ""

    text: layoutNames.longName
    visible: hasMultipleKeyboardLayouts

    Accessible.name: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Button to change keyboard layout", "Switch layout")
    icon.name: "input-keyboard"

    onClicked: keyboardLayout.switchToNextLayout()

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton

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
    }

    KeyboardLayout {
        id: keyboardLayout
    }
}
