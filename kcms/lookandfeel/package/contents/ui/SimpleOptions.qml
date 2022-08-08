/*
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

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
            id: appearanceSettingsCheckbox
            text: i18n("Appearance settings")
            checked: kcm.appearanceToApply & Private.LookandFeelManager.AppearanceSettings
            onToggled: kcm.appearanceToApply ^= Private.LookandFeelManager.AppearanceSettings
            enabled: view.model.data(view.model.index(view.currentIndex, 0), Private.KCMLookandFeel.HasGlobalThemeRole) //enabled is needed because lblsub can make it invisible
            visible: enabled && !appearanceSettingsCheckboxLblSub.visible
        }
        QtControls.Label { //These labels sub in for the checkboxes when they're the only visible checkbox in this page
            id: appearanceSettingsCheckboxLblSub
            visible: !resetCheckbox.enabled && appearanceSettingsCheckbox.enabled
            Layout.fillWidth: true
            text: i18nc("List item", "• Appearance settings")
            wrapMode: Text.WordWrap
        }
        QtControls.CheckBox {
            id: resetCheckbox
            text: i18n("Desktop and window layout")
            checked: kcm.layoutToApply & Private.LookandFeelManager.LayoutSettings
            onToggled: kcm.layoutToApply ^= Private.LookandFeelManager.LayoutSettings
            enabled: view.model.data(view.model.index(view.currentIndex, 0), Private.KCMLookandFeel.HasLayoutSettingsRole) ||
                view.model.data(view.model.index(view.currentIndex, 0), Private.KCMLookandFeel.HasDesktopLayoutRole)
            visible: enabled && !resetCheckboxLblSub.visible
        }
        QtControls.Label {
            id: resetCheckboxLblSub
            visible: resetCheckbox.enabled && !appearanceSettingsCheckbox.enabled
            Layout.fillWidth: true
            text: i18nc("List item", "• Desktop and window layout")
            wrapMode: Text.WordWrap
        }
        QtControls.Label {
            Layout.fillWidth: true
            text: i18n("Applying a Desktop layout replaces your current configuration of desktops, panels, docks, and widgets")
            elide: Text.ElideRight
            wrapMode: Text.WordWrap
            font: Kirigami.Theme.smallFont
            visible: resetCheckbox.visible || resetCheckboxLblSub.visible
            color: Kirigami.Theme.neutralTextColor
        }
        QtControls.Label { //This label shouldn't ever appear, but it's good to let the user know
            //why the dialog has no options in the rare scenario they provide an
            //an empty Global Theme
            visible: !resetCheckbox.visible && !appearanceSettingsCheckbox.visible && !resetCheckboxLblSub.visible && !appearanceSettingsCheckboxLblSub.visible
            Layout.fillWidth: true
            text: i18n("This Global Theme does not provide any applicable settings. Please contact the maintainer of this Global Theme as it might be broken.")
            wrapMode: Text.WordWrap
        }
    }
}
