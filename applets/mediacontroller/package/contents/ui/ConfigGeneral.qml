/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.private.mediacontroller as Private

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: configRoot

    property bool cfg_multiplexerEnabled
    property bool cfg_useGlobalPreferredPlayer
    property string cfg_localPreferredPlayer

    Private.PlayerHistoryModel {
        id: historyModel
    }

    Component {
        id: nameDialog

        Kirigami.PromptDialog {
            required property Item configRoot
            parent: configRoot
            title: i18nc("@title:window here 'player' means 'media player app'", "Choose Other Player")
            standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

            onAccepted: if (textField.text.length > 0) {
                historyModel.rememberPlayer(textField.text);
                configRoot.cfg_localPreferredPlayer = textField.text;
            }
            onRejected: {
                configRoot.cfg_localPreferredPlayer = "";
            }
            onClosed: destroy()

            Kirigami.ActionTextField {
                id: textField
                focus: true
                placeholderText: i18nc("@label", "Enter a player name, e.g org.kde.elisa")
            }

            Component.onCompleted: open()
        }
    }

    Kirigami.FormLayout {
        Layout.fillHeight: true

        QQC.ButtonGroup {
            buttons: [followGlobalRadioButon, useLocalRadioButton, multiplexerEnabledCheckBox]
        }

        QQC.RadioButton {
            id: followGlobalRadioButon
            Kirigami.FormData.label: i18nc("@label", "Automatically switch to:")
            Layout.fillWidth: false
            text: i18nc("@option:radio choose music player automatically", "Most recently used")
            checked: configRoot.cfg_useGlobalPreferredPlayer
            onToggled: {
                configRoot.cfg_multiplexerEnabled = true;
                configRoot.cfg_useGlobalPreferredPlayer = followGlobalRadioButon.checked;
            }
        }

        QQC.RadioButton {
            id: useLocalRadioButton
            Layout.fillWidth: false
            text: i18nc("@option:radio here 'player' means 'music player app'", "Preferred player when available:")
            checked: !configRoot.cfg_useGlobalPreferredPlayer
            onToggled: {
                configRoot.cfg_multiplexerEnabled = true;
                configRoot.cfg_useGlobalPreferredPlayer = !useLocalRadioButton.checked;
            }
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC.ComboBox {
                id: playerComboBox
                Layout.leftMargin: Kirigami.Units.gridUnit
                Layout.minimumWidth: useLocalRadioButton.implicitWidth - Kirigami.Units.gridUnit
                enabled: useLocalRadioButton.enabled && useLocalRadioButton.checked
                model: historyModel
                textRole: "display"
                valueRole: "identity"
                currentIndex: {
                    console.log(count, configRoot.cfg_localPreferredPlayer)
                    return count, historyModel.indexOf(configRoot.cfg_localPreferredPlayer)
                }

                delegate: QQC.ItemDelegate {
                    required property QtObject model
                    width: ListView.view.width
                    text: model.display
                    icon.name: model.decoration
                }

                onActivated: index => {
                    if (index == count - 1) {
                        nameDialog.createObject(configRoot, {
                            "configRoot": configRoot,
                        });
                        return;
                    }
                    configRoot.cfg_localPreferredPlayer = currentValue;
                }

                // HACK QQC doesn't support icons, so we just tamper with the desktop style ComboBox's background
                Component.onCompleted: {
                    if (!background || !background.hasOwnProperty("properties")) {
                        //not a KQuickStyleItem
                        return;
                    }
                    const props = background.properties || {};
                    background.properties = Qt.binding(() => {
                        const modelIndex = model.index(currentIndex, 0);
                        const currentIcon = model.data(modelIndex, Qt.DecorationRole);
                        return Object.assign(props, {
                            currentIcon,
                            iconColor: Kirigami.Theme.textColor,
                        });
                    });
                }
            }

            QQC.Button {
                enabled: playerComboBox.enabled && playerComboBox.count > 1
                display: QQC.AbstractButton.IconOnly
                icon.name: "edit-clear-all"
                text: i18nc("@action:button here 'player' means 'music player app'", "Forget all players")
                onClicked: historyModel.forgetAllPlayers()
                QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
                QQC.ToolTip.text: text
                QQC.ToolTip.visible: enabled && ((Kirigami.Settings.tabletMode ? pressed : hovered) || activeFocus)
            }
        }

        QQC.RadioButton {
            id: multiplexerEnabledCheckBox
            Layout.fillWidth: false
            text: i18nc("@option:radio", "No automatic switching")
            checked: !configRoot.cfg_multiplexerEnabled
            onToggled: configRoot.cfg_multiplexerEnabled = !checked
        }
    }
}
