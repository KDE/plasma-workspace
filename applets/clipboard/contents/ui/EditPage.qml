/*
    SPDX-FileCopyrightText: 2021 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras

ColumnLayout {
    spacing: 0
    property alias text: textArea.text
    property string uuid

    property var header: Item {}

    Keys.onPressed: {
        if (event.key === Qt.Key_Escape) {
            stack.pop()
            done();
            event.accepted = true;
        }
    }

    function saveAndExit() {
        clipboardSource.edit(uuid, text);
        stack.pop();
        done();
    }

    Component.onCompleted: {
        textArea.forceActiveFocus();
        textArea.cursorPosition = textArea.text.length;
    }

    function done() {
        stack.initialItem.view.currentIndex = 0;
        stack.initialItem.view.currentItem.forceActiveFocus();
    }

    PlasmaComponents3.TextArea {
        id: textArea
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: PlasmaCore.Units.smallSpacing * 2
        Layout.rightMargin: PlasmaCore.Units.smallSpacing * 2
        Layout.topMargin: PlasmaCore.Units.smallSpacing * 2

        Keys.onPressed: {
            if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                saveAndExit();
                event.accepted = true;
            } else {
                event.accepted = false;
            }
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignRight
        Layout.margins: PlasmaCore.Units.smallSpacing * 2
        PlasmaComponents3.Button {
            text: i18nc("@action:button", "Save")
            icon.name: "document-save"
            onClicked: saveAndExit()
        }
        PlasmaComponents3.Button {
            text: i18nc("@action:button", "Cancel")
            icon.name: "dialog-cancel"
            onClicked: { stack.pop(); done() }
        }
    }
}
