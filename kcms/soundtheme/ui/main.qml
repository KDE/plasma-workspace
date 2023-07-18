/*
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM


KCM.GridViewKCM {
    implicitWidth: Kirigami.Units.gridUnit * 50
    implicitHeight: Kirigami.Units.gridUnit * 30

    KCM.SettingStateBinding {
        configObject: kcm.settings
        settingName: "theme"
    }

    view.implicitCellWidth: Kirigami.Units.gridUnit * 22
    view.implicitCellHeight: Kirigami.Units.gridUnit * 11

    view.model: kcm.themes
    view.currentIndex: kcm.currentIndex
    view.delegate: KCM.GridDelegate {
        id: delegate

        readonly property var theme : modelData
        readonly property bool compactMode: width < (preview.implicitWidth + demoButton.implicitWidth + 3 * Kirigami.Units.gridUnit)

        text: theme.name

        thumbnailAvailable: true
        thumbnail: GridLayout {
            columns: 2
            flow: GridLayout.TopToBottom
            anchors.margins: Kirigami.Units.largeSpacing
            anchors.fill: parent

            Kirigami.Heading {
                text: theme.name
                level: 3
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            QQC2.Label {
                text: theme.comment
                opacity: 0.6
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            QQC2.Label {
                visible: theme.inherits.length > 0
                // We add a plural translation in case some language requires different strings
                text: i18ncp("%2 is a theme name or a list of theme names that the current theme inherits from",
                             "Based on: %2", "Based on: %2", theme.inherits.length,
                            theme.inherits.map(themeId => kcm.nameFor(themeId)).join(', '))
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Item {
                Layout.fillHeight: true
            }
            RowLayout {
                id: preview
                QQC2.Label {
                    text: i18nc("@label Precedes a list of buttons which can be clicked to preview the theme's sounds. Keep it short", "Preview sounds:")
                }
                Repeater {
                    model: [
                        {iconName: "preferences-desktop-notification-bell", sounds: ["bell-window-system"]},
                        {iconName: "dialog-warning", sounds: ["dialog-warning", "dialog-information"]},
                        {iconName: "message-new", sounds: ["message-new-instant", "message-new-email"]},
                        {iconName: "battery-low", sounds: ["battery-caution", "battery-low"]},
                        {iconName: "device-notifier", sounds: ["device-added", "device-removed"]},
                    ]
                    delegate: QQC2.ToolButton {
                        icon.name: modelData.iconName
                        text: i18nc("@info:tooltip", "Preview sound \"%1\"", modelData.sounds[0])
                        display: QQC2.AbstractButton.IconOnly
                        onClicked: kcm.playSound(theme.id, modelData.sounds)

                        Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                        Layout.preferredHeight: Kirigami.Units.iconSizes.medium

                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.visible: hovered || activeFocus
                    }
                }
                Layout.row: 4
                Layout.columnSpan: delegate.compactMode ? 2 : 1
            }

            QQC2.ToolButton {
                id: demoButton

                icon.name: "media-playback-start-symbolic"
                text: i18nc("@info:tooltip", "Preview the demo sound for the theme \"%1\"", theme.name)
                display: QQC2.AbstractButton.IconOnly
                onClicked: kcm.playDemo(theme)

                Layout.column: 1
                Layout.rowSpan: delegate.compactMode ? 4 : 5
                Layout.preferredWidth: Kirigami.Units.iconSizes.large
                Layout.fillHeight: true

                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered || activeFocus
            }
        }

        TapHandler {
            onTapped: kcm.settings.theme = theme.id
        }
    }
}
