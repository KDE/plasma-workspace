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

    actions: Kirigami.Action {
        id: enabledAction
        icon.name: "notification-active-symbolic"
        text: i18nc("@option:check", "Enable notification sounds")
        checkable: true
        checked: kcm.settings.soundsEnabled

        displayComponent: QQC2.CheckBox {
            text: enabledAction.text
            checked: enabledAction.checked
            onToggled: kcm.settings.soundsEnabled = checked

            // HACK: Kirigami.ToolBarPageHeader shows no padding otherwise
            rightPadding: Kirigami.Units.smallSpacing

            KCM.SettingStateBinding {
                configObject: kcm.settings
                settingName: "soundsEnabled"
            }
        }
    }

    view.enabled: kcm.settings.soundsEnabled
    view.model: kcm.themes
    view.currentIndex: kcm.currentIndex

    header: Kirigami.InlineMessage {
        id: errorMessage
        Layout.fillWidth: true
        showCloseButton: true

        function display(errorText, errorType) {
            text = errorText
            type = errorType ?? Kirigami.MessageType.Error;
            visible = true;
        }
    }

    view.delegate: KCM.GridDelegate {
        id: delegate

        readonly property var theme : modelData

        text: theme.name

        thumbnailAvailable: true
        thumbnail: GridLayout {
            columns: 2
            flow: GridLayout.TopToBottom
            anchors.margins: Kirigami.Units.largeSpacing
            anchors.fill: parent

            Kirigami.Heading {
                text: theme.name
                textFormat: Text.PlainText
                level: 3
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            QQC2.Label {
                text: theme.comment
                textFormat: Text.PlainText
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
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Item {
                Layout.fillHeight: true
            }
            RowLayout {
                id: preview
                spacing: Kirigami.Units.smallSpacing
                QQC2.Label {
                    text: i18nc("@label Precedes a list of buttons which can be clicked to preview the theme's sounds. Keep it very short", "Preview:")
                    textFormat: Text.PlainText
                    // Make the Label able to shrink and elide when needed
                    elide: Text.ElideLeft
                    Layout.fillWidth: true
                    Layout.maximumWidth: implicitWidth + Kirigami.Units.smallSpacing
                }
                Repeater {
                    model: [
                        {iconName: "preferences-desktop-notification-bell", sounds: ["bell-window-system"]},
                        {iconName: "dialog-warning", sounds: ["dialog-warning", "dialog-information"]},
                        {iconName: "message-new", sounds: ["message-new-instant", "message-new-email"]},
                        {iconName: "battery-low", sounds: ["battery-caution", "battery-low"]},
                        {iconName: "device-notifier", sounds: ["device-added", "device-removed"]},
                    ]
                    delegate: SoundButton {
                        themeId: theme.id
                        sounds: modelData.sounds
                        icon.name: modelData.iconName
                        text: i18nc("@info:tooltip", "Preview sound \"%1\"", sounds[0])
                    }
                }
                Layout.row: 4
                Layout.columnSpan: 2
            }

            SoundButton {
                id: demoButton

                themeId: theme.id
                sounds: {
                    // We try to play the demo sound specified by the theme, either in `index.theme`
                    // or as a named sound ("theme-demo"), but if none is provided let's fallback
                    // to other common descriptive sounds
                    const demoSounds = ["theme-demo", "desktop-login", "service-login"];
                    if (theme.example.length > 0) {
                        demoSounds.unshift(theme.example);
                    }
                    return demoSounds;
                }
                icon.name: isPlaying ? "media-playback-stop-symbolic": "media-playback-start-symbolic"
                text: i18nc("@info:tooltip", "Preview the demo sound for the theme \"%1\"", theme.name)

                Layout.column: 1
                Layout.rowSpan: 4
                Layout.preferredWidth: Kirigami.Units.iconSizes.large
                Layout.fillHeight: true
            }
        }

        TapHandler {
            onTapped: kcm.settings.theme = theme.id
        }
    }

    component SoundButton: QQC2.ToolButton {
        property string themeId: ""
        property var sounds: []
        readonly property bool isPlaying: kcm.playingTheme === themeId && sounds.includes(kcm.playingSound)

        display: QQC2.AbstractButton.IconOnly
        down: isPlaying  // We just want the visual cue, not the checked state
        onClicked: {
            if (isPlaying) {
                kcm.cancelSound();
                return;
            }

            const result = kcm.playSound(themeId, sounds);

            switch (result) {
            case 0: /* CA_ERROR_SUCCESS */
                errorMessage.visible = false;
                break;
            case -9: /* CA_ERROR_NOTFOUND */
                errorMessage.display(i18nc("%1 is a sound theme name. %2 is a sound name",
                                           "The %1 theme doesn't include the sound '%2'",
                                           kcm.nameFor(themeId), sounds[0]),
                                     Kirigami.MessageType.Information);
                break;
            default:
                errorMessage.display(i18n("Failed to preview sound: %1", kcm.errorString(result)));
            }
        }

        Layout.preferredWidth: Kirigami.Units.iconSizes.medium
        Layout.preferredHeight: Kirigami.Units.iconSizes.medium

        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: (hovered || activeFocus) && enabled
    }
}
