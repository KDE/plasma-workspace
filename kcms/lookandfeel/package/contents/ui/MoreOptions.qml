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
    Kirigami.FormLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing

        ColumnLayout {
            Kirigami.FormData.label: i18n("Layout settings:")
            visible: root.hasLayout
            Repeater {
                model: [
                    { text: i18n("Desktop layout"),
                      role: Private.KCMLookandFeel.HasDesktopLayoutRole,
                      flag: Private.LookandFeelManager.DesktopLayout
                            | Private.LookandFeelManager.DesktopSwitcher
                            | Private.LookandFeelManager.WindowPlacement
                            | Private.LookandFeelManager.ShellPackage
                    },
                    { text: i18n("Titlebar Buttons layout"), role: Private.KCMLookandFeel.HasTitlebarLayoutRole, flag: Private.LookandFeelManager.TitlebarLayout },
                ]
                delegate: QtControls.CheckBox {
                    required property var modelData
                    text: modelData.text
                    checked: kcm.layoutToApply & modelData.flag
                    visible: view.model.data(view.model.index(view.currentIndex, 0), modelData.role)
                    onToggled: kcm.layoutToApply ^= modelData.flag
                }
            }
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

        ColumnLayout {
            Kirigami.FormData.label: i18n("Appearance settings:")
            visible: root.hasAppearance
            Repeater {
                model: [
                    { text: i18n("Colors"), role: Private.KCMLookandFeel.HasColorsRole, flag: Private.LookandFeelManager.Colors },
                    { text: i18n("Application Style"), role: Private.KCMLookandFeel.HasWidgetStyleRole, flag: Private.LookandFeelManager.WidgetStyle },
                    { text: i18n("Window Decorations"), role: Private.KCMLookandFeel.HasWindowDecorationRole, flag: Private.LookandFeelManager.WindowDecoration },
                    { text: i18n("Icons"), role: Private.KCMLookandFeel.HasIconsRole, flag: Private.LookandFeelManager.Icons },
                    { text: i18n("Plasma Style"), role: Private.KCMLookandFeel.HasPlasmaThemeRole, flag: Private.LookandFeelManager.PlasmaTheme },
                    { text: i18n("Cursors"), role: Private.KCMLookandFeel.HasCursorsRole, flag: Private.LookandFeelManager.Cursors },
                    { text: i18n("Fonts"), role: Private.KCMLookandFeel.HasFontsRole, flag: Private.LookandFeelManager.Fonts },
                    { text: i18n("Task Switcher"), role: Private.KCMLookandFeel.HasWindowSwitcherRole, flag: Private.LookandFeelManager.WindowSwitcher },
                    { text: i18n("Splash Screen"), role: Private.KCMLookandFeel.HasSplashRole, flag: Private.LookandFeelManager.SplashScreen },
                    { text: i18n("Lock Screen"), role: Private.KCMLookandFeel.HasLockScreenRole, flag: Private.LookandFeelManager.LockScreen },
                ]
                delegate: QtControls.CheckBox {
                    required property var modelData
                    text: modelData.text
                    checked: kcm.appearanceToApply & modelData.flag
                    visible: view.model.data(view.model.index(view.currentIndex, 0), modelData.role)
                    onToggled: kcm.appearanceToApply ^= modelData.flag
                }
            }
        }
    }
}
