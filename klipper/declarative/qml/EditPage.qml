/*
    SPDX-FileCopyrightText: 2021 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Templates as T // For StackView
import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.private.clipboard 0.1 as Private

ColumnLayout {
    id: editPage
    spacing: 0

    required property PlasmaExtras.Representation dialogItem
    required property Private.HistoryModel historyModel
    required property var modelData
    property alias text: textArea.text

    property Item header: null

    Keys.onEscapePressed: T.StackView.view.popCurrentItem()

    function saveAndExit() {
        modelData.display = text;
        historyModel.moveToTop(modelData.uuid);
        T.StackView.view.popCurrentItem();
        T.StackView.view.initialItem.view.currentIndex = 0;
    }

    T.StackView.onStatusChanged: {
        if (editPage.T.StackView.status === T.StackView.Active) {
            textArea.forceActiveFocus(Qt.ActiveWindowFocusReason);
            textArea.cursorPosition = textArea.text.length;
        }
    }

    Shortcut {
        sequence: StandardKey.Save
        onActivated: editPage.saveAndExit()
    }

    PlasmaComponents3.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: Kirigami.Units.smallSpacing * 2
        Layout.rightMargin: PlasmaComponents3.ScrollBar.vertical.visible ? 0 : Kirigami.Units.smallSpacing * 2
        Layout.topMargin: Kirigami.Units.smallSpacing * 2

        PlasmaComponents3.TextArea {
            id: textArea
            wrapMode: TextEdit.Wrap
            textFormat: TextEdit.PlainText
            text: editPage.modelData?.display ?? ""

            KeyNavigation.up: editPage.dialogItem.KeyNavigation.up
            Keys.onPressed: event => {
                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                    editPage.saveAndExit();
                    event.accepted = true;
                } else {
                    event.accepted = false;
                }
            }
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignRight
        Layout.margins: Kirigami.Units.smallSpacing * 2
        PlasmaComponents3.Button {
            text: i18ndc("klipper", "@action:button", "Save")
            icon.name: "document-save"
            KeyNavigation.up: textArea
            KeyNavigation.right: cancelButton
            onClicked: editPage.saveAndExit()
        }
        PlasmaComponents3.Button {
            id: cancelButton
            text: i18ndc("klipper", "@action:button", "Cancel")
            icon.name: "dialog-cancel"
            KeyNavigation.up: textArea
            onClicked: editPage.T.StackView.view.popCurrentItem()
        }
    }
}
