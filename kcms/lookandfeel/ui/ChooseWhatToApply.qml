/*
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import org.kde.private.kcms.lookandfeel as Private
import org.kde.kcmutils as KCM

KCM.AbstractKCM {
    id: root

    readonly property bool hasAppearance: kcm.themeContents & Private.LookandFeelManager.AppearanceSettings
    readonly property bool hasLayout: kcm.themeContents & Private.LookandFeelManager.LayoutSettings
    readonly property bool showLayoutInfo: kcm.themeContents & Private.LookandFeelManager.DesktopLayout

    title: i18nc("@title", "Choose what to apply")

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        Kirigami.FormLayout {
            id: layoutForm
            twinFormLayouts: root.hasAppearance ? appearanceForm : null
            visible: root.hasLayout
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing

            ColumnLayout {
                Kirigami.FormData.label: i18nc("@label", "Layout settings:")
                Repeater {
                    model: [
                        { text: i18nc("@option:check", "Desktop layout"),
                        flag: Private.LookandFeelManager.DesktopLayout
                                | Private.LookandFeelManager.WindowPlacement
                                | Private.LookandFeelManager.ShellPackage,
                        enabled: !kcm.plasmaLocked
                        },
                        { text: i18nc("@option:check", "Titlebar Buttons layout"), flag: Private.LookandFeelManager.TitlebarLayout, enabled: true },
                    ]
                    delegate: QtControls.CheckBox {
                        required property var modelData
                        text: modelData.text
                        visible: enabled && (kcm.themeContents & modelData.flag)
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
            text: i18nc("@info", "Applying a Desktop layout will delete the current set of desktops, panels, docks, and widgets, replacing them with what the theme specifies.")
        }

        Kirigami.FormLayout {
            id: appearanceForm
            twinFormLayouts: root.hasLayout ? layoutForm : null
            visible: root.hasAppearance
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing

            ColumnLayout {
                Kirigami.FormData.label: i18nc("@label", "Appearance settings:")
                Repeater {
                    model: [
                        { text: i18nc("@option:check", "Colors"), flag: Private.LookandFeelManager.Colors },
                        { text: i18nc("@option:check", "Application Style"), flag: Private.LookandFeelManager.WidgetStyle },
                        { text: i18nc("@option:check", "Window Decoration Style"), flag: Private.LookandFeelManager.WindowDecoration },
                        { text: i18nc("@option:check", "Window Decoration Size"), flag: Private.LookandFeelManager.BorderSize },
                        { text: i18nc("@option:check", "Icons"), flag: Private.LookandFeelManager.Icons },
                        { text: i18nc("@option:check", "Plasma Style"), flag: Private.LookandFeelManager.PlasmaTheme },
                        { text: i18nc("@option:check", "Cursors"), flag: Private.LookandFeelManager.Cursors },
                        { text: i18nc("@option:check", "Fonts"), flag: Private.LookandFeelManager.Fonts },
                        { text: i18nc("@option:check", "Task Switcher"), flag: Private.LookandFeelManager.WindowSwitcher },
                        { text: i18nc("@option:check", "Splash Screen"), flag: Private.LookandFeelManager.SplashScreen },
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

        Item {
            Layout.fillHeight: true
        }
    }
}
