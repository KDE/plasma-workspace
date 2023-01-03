/*
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Controls 2.3 as QtControls
import org.kde.kirigami 2.8 as Kirigami
import org.kde.private.kcms.lookandfeel 1.0 as Private

ColumnLayout {
    id: main

    Kirigami.Heading {
        Layout.fillWidth: true
        text: i18n("The following will be applied by this Global Theme:")
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
            checked: kcm.appearanceToApply & Private.LookandFeelManager.AppearanceSettings
            onToggled: kcm.appearanceToApply ^= Private.LookandFeelManager.AppearanceSettings
            enabled: view.model.data(view.model.index(view.currentIndex, 0), Private.KCMLookandFeel.HasGlobalThemeRole) //enabled is needed because lblsub can make it invisible
        }
        QtControls.Label { //These labels sub in for the checkboxes when they're the only visible checkbox in this page
            Layout.fillWidth: true
            visible: root.hasAppearance && !root.hasLayout
            text: i18nc("List item", "• Appearance settings")
            wrapMode: Text.WordWrap
        }

        QtControls.CheckBox {
            visible: root.hasAppearance && root.hasLayout
            text: i18n("Desktop and window layout")
            checked: kcm.layoutToApply & Private.LookandFeelManager.LayoutSettings
            onToggled: kcm.layoutToApply ^= Private.LookandFeelManager.LayoutSettings
            enabled: view.model.data(view.model.index(view.currentIndex, 0), Private.KCMLookandFeel.HasLayoutSettingsRole) ||
                view.model.data(view.model.index(view.currentIndex, 0), Private.KCMLookandFeel.HasDesktopLayoutRole)
        }
        QtControls.Label {
            Layout.fillWidth: true
            visible: root.hasLayout && !root.hasAppearance
            text: i18nc("List item", "• Desktop and window layout")
            wrapMode: Text.WordWrap
        }

        QtControls.Label {
            Layout.fillWidth: true
            visible: root.showLayoutInfo
            text: i18n("Applying a Desktop layout replaces your current configuration of desktops, panels, docks, and widgets")
            elide: Text.ElideRight
            wrapMode: Text.WordWrap
            font: Kirigami.Theme.smallFont
            color: Kirigami.Theme.neutralTextColor
        }
        // This label shouldn't ever appear, but it's good to let the user know why the dialog has no options
        // in the rare scenario they provide an empty Global Theme
        QtControls.Label {
            Layout.fillWidth: true
            visible: !root.hasAppearance && !root.hasLayout
            text: i18n("This Global Theme does not provide any applicable settings. Please contact the maintainer of this Global Theme as it might be broken.")
            wrapMode: Text.WordWrap
        }
    }
}
