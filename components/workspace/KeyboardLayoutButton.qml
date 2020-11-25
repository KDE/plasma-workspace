/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 * SPDX-FileCopyrightText: 2020 Andrey Butirsky <butirsky@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.12
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.workspace.keyboardlayout 1.0

PlasmaComponents3.ToolButton {
    id: kbLayoutButton

    property alias layoutShortName: layout.layoutDisplayName
    property alias layoutLongName: layout.layoutLongName
    readonly property bool hasMultipleKeyboardLayouts: layout.layouts.length > 1

    text: layoutLongName
    visible: hasMultipleKeyboardLayouts

    Accessible.name: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Button to change keyboard layout", "Switch layout")
    icon.name: "input-keyboard"

    onClicked: layout.switchToNextLayout()

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        onWheel: {
            if (wheel.angleDelta.y > 0) {
                layout.switchToNextLayout()
            } else {
                layout.switchToPreviousLayout()
            }
        }
    }

    KeyboardLayout {
        id: layout
    }
}
