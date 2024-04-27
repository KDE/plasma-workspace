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
    Kirigami.FormLayout {
        id: layoutForm
        twinFormLayouts: root.hasAppearance ? appearanceForm : null
        visible: root.hasLayout
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing

        ColumnLayout {
            Kirigami.FormData.label: i18n("Layout settings:")
            Repeater {
                model: [
                    { text: i18n("Desktop layout"),
                      flag: Private.LookandFeelManager.DesktopLayout
                            | Private.LookandFeelManager.WindowPlacement
                            | Private.LookandFeelManager.ShellPackage
                    },
                    { text: i18n("Titlebar Buttons layout"), flag: Private.LookandFeelManager.TitlebarLayout },
                ]
                delegate: QtControls.CheckBox {
                    required property var modelData
                    text: modelData.text
                    visible: kcm.themeContents & modelData.flag
                    checked: kcm.selectedContents & modelData.flag
                    onToggled: {
                        kcm.selectedContents ^= modelData.flag
                        if (modelData.flag | Private.LookandFeelManager.DesktopLayout) {
                            resetLayoutWarning.visible = checked;
                        }
                    }
                }
            }
        }
    }

    Kirigami.InlineMessage {
        id: resetLayoutWarning
        Layout.fillWidth: true
        visible: false
        type: Kirigami.MessageType.Warning
        text: i18n("Applying a Desktop layout replaces your current configuration of desktops, panels, docks, and widgets.")
    }

    Kirigami.FormLayout {
        id: appearanceForm
        twinFormLayouts: root.hasLayout ? layoutForm : null
        visible: root.hasAppearance
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing

        ColumnLayout {
            Kirigami.FormData.label: i18n("Appearance settings:")
            Repeater {
                model: [
                    { text: i18n("Colors"), flag: Private.LookandFeelManager.Colors },
                    { text: i18n("Application Style"), flag: Private.LookandFeelManager.WidgetStyle },
                    { text: i18n("Window Decoration Style"), flag: Private.LookandFeelManager.WindowDecoration },
                    { text: i18n("Window Decoration Size"), flag: Private.LookandFeelManager.BorderSize },
                    { text: i18n("Icons"), flag: Private.LookandFeelManager.Icons },
                    { text: i18n("Plasma Style"), flag: Private.LookandFeelManager.PlasmaTheme },
                    { text: i18n("Cursors"), flag: Private.LookandFeelManager.Cursors },
                    { text: i18n("Fonts"), flag: Private.LookandFeelManager.Fonts },
                    { text: i18n("Task Switcher"), flag: Private.LookandFeelManager.WindowSwitcher },
                    { text: i18n("Splash Screen"), flag: Private.LookandFeelManager.SplashScreen },
                ]
                delegate: QtControls.CheckBox {
                    required property var modelData
                    text: modelData.text
                    visible: kcm.themeContents & modelData.flag
                    checked: kcm.selectedContents & modelData.flag
                    onToggled: kcm.selectedContents ^= modelData.flag
                }
            }
        }
    }
}
