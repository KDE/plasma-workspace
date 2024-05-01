/*
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Controls 2.3 as QtControls
import org.kde.kirigami as Kirigami
import org.kde.private.kcms.lookandfeel 1.0 as Private

ColumnLayout {

    Kirigami.Heading {
        Layout.fillWidth: true
        text: i18n("The following will be applied by this Global Theme:")
        textFormat: Text.PlainText
        level: 2
        wrapMode: Text.WordWrap
    }

    Kirigami.FormLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing

        QtControls.CheckBox {
            visible: root.hasAppearance && root.hasLayout
            text: i18n("Appearance settings")
            checked: kcm.selectedContents & Private.LookandFeelManager.AppearanceSettings
            onToggled: kcm.selectedContents ^= Private.LookandFeelManager.AppearanceSettings
        }
        QtControls.Label { // These labels sub in for the checkboxes when they're the only visible checkbox in this page
            Layout.fillWidth: true
            visible: root.hasAppearance && !root.hasLayout
            text: i18nc("List item", "• Appearance settings")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }

        QtControls.CheckBox {
            id: resetLayoutCheckbox
            visible: root.hasAppearance && root.hasLayout
            text: i18n("Desktop and window layout")
            checked: kcm.selectedContents & Private.LookandFeelManager.LayoutSettings
            onToggled: kcm.selectedContents ^= Private.LookandFeelManager.LayoutSettings
        }
        QtControls.Label {
            Layout.fillWidth: true
            visible: root.hasLayout && !root.hasAppearance
            text: i18nc("List item", "• Desktop and window layout")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: resetLayoutCheckbox.checked
            type: Kirigami.MessageType.Warning
            text: i18n("Applying a Desktop layout will delete the current set of desktops, panels, docks, and widgets, replacing them with what the theme specifies.")
        }
        // This label shouldn't ever appear, but it's good to let the user know why the dialog has no options
        // in the rare scenario they provide an empty Global Theme
        QtControls.Label {
            Layout.fillWidth: true
            visible: !root.hasAppearance && !root.hasLayout
            text: i18n("This Global Theme does not provide any applicable settings. Please contact the maintainer of this Global Theme as it might be broken.")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }
    }
}
