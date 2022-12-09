/*
    SPDX-FileCopyrightText: 2021 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2 // For StackView
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
            event.accepted = true;
        }
    }

    function saveAndExit() {
        clipboardSource.edit(uuid, text);
        stack.pop();
        done();
    }

    function done() {
        // The modified item will be pushed to the top, and we would like to highlight the real first item
        Qt.callLater(() => {stack.initialItem.view.currentIndex = 0;});
    }

    QQC2.StackView.onStatusChanged: {
        if (QQC2.StackView.status === QQC2.StackView.Active) {
            textArea.forceActiveFocus(Qt.ActiveWindowFocusReason);
            textArea.cursorPosition = textArea.text.length;
            main.editing = true;
        } else {
            main.editing = false;
        }
    }

    Shortcut {
        sequence: StandardKey.Save
        onActivated: saveAndExit()
    }

    PlasmaComponents3.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: PlasmaCore.Units.smallSpacing * 2
        Layout.rightMargin: PlasmaComponents3.ScrollBar.vertical.visible ? 0 : PlasmaCore.Units.smallSpacing * 2
        Layout.topMargin: PlasmaCore.Units.smallSpacing * 2

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        PlasmaComponents3.TextArea {
            id: textArea
            wrapMode: Text.Wrap
            textFormat: TextEdit.PlainText

            KeyNavigation.up: dialogItem.KeyNavigation.up
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
        Layout.margins: PlasmaCore.Units.smallSpacing * 2
        PlasmaComponents3.Button {
            text: i18nc("@action:button", "Save")
            icon.name: "document-save"
            KeyNavigation.up: textArea
            KeyNavigation.right: cancelButton
            onClicked: saveAndExit()
        }
        PlasmaComponents3.Button {
            id: cancelButton
            text: i18nc("@action:button", "Cancel")
            icon.name: "dialog-cancel"
            KeyNavigation.up: textArea
            onClicked: stack.pop()
        }
    }
}
