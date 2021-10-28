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
    property alias text: textArea.text
    required property string uuid

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
        header.enabled = false;
    }

    function done() {
        stack.initialPage.view.currentIndex = 0;
        stack.initialPage.view.currentItem.forceActiveFocus();
        header.enabled = true;
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.topMargin: PlasmaCore.Units.smallSpacing

        PlasmaComponents3.TextArea {
            id: textArea
            anchors.fill: parent
            Keys.onPressed: {
                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                    saveAndExit();
                    event.accepted = true;
                } else {
                    event.accepted = false;
                }
            }
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignRight
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
