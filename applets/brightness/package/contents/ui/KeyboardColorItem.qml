/*
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as QtDialogs

import org.kde.kcmutils // KCMLauncher
import org.kde.config as KConfig  // KAuthorized.authorizeControlModule
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

import org.kde.plasma.private.brightnesscontrolplugin

PlasmaComponents3.ItemDelegate {
    id: root

    background.visible: highlighted
    highlighted: activeFocus
    hoverEnabled: false

    // visible: keyboardColorControl.supported

    contentItem: RowLayout {
        spacing: Kirigami.Units.gridUnit

        Kirigami.Icon {
            id: icon
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: Kirigami.Units.iconSizes.medium
            source: "input-keyboard-color"
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.Label {
                    id: title
                    text: root.text
                    textFormat: Text.PlainText

                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Rectangle {
                    id: colorIndicator

                    width: Math.round(Kirigami.Units.gridUnit * 1.25)
                    height: Math.round(Kirigami.Units.gridUnit * 1.25)
                    radius: 180

                    color: keyboardColorControl.color
                    border.color: Qt.rgba(0, 0, 0, 0.15)

                    Connections {
                        target: keyboardColorControl
                        onColorChanged: colorIndicator.color = keyboardColorControl.color
                    }

                    QQC2.RoundButton {
                        id: colorPickerButton
                        visible: !syncAccentSwitch.checked
                        icon.name: "color-picker"

                        height: colorIndicator.height
                        width: colorIndicator.width
                        padding: 0  // Round button adds some padding by default which we don't need. Setting this to 0 centers the icon
                        icon.width : Math.round(Kirigami.Units.iconSizes.small * 0.9) // This provides a nice padding
                        icon.height : Math.round(Kirigami.Units.iconSizes.small * 0.9)

                        onClicked: colorDialog.open()
                    }
                }
            }

            RowLayout {
                PlasmaComponents3.Label { // label for off switch state
                    text: i18n("Choose custom color")
                }
                PlasmaComponents3.Switch {
                    id: syncAccentSwitch
                    checked: keyboardColorControl.accent
                    text: i18n("Follow accent color")

                    Layout.fillWidth: true

                    KeyNavigation.up: root.KeyNavigation.up
                    KeyNavigation.tab: colorPicker.enabled ? colorPicker : root.KeyNavigation.tab
                    KeyNavigation.right: colorPicker.enabled ? colorPicker : root.KeyNavigation.right
                    KeyNavigation.backtab: root.KeyNavigation.backtab

                    Keys.onPressed: (event) => {
                        if (event.key == Qt.Key_Space || event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                            toggle();
                        }
                    }
                    onToggled: {
                        keyboardColorControl.setAccent(checked);
                        colorIndicator.color = "transparent"
                    }
                }

                QtDialogs.ColorDialog {
                    id: colorDialog
                    title: i18n("Choose custom keyboard color")
                    // User must either choose a colour or cancel the operation before doing something else
                    modality: Qt.ApplicationModal
                    parentWindow: root.Window.window
                    selectedColor: keyboardColorControl.color
                    onAccepted: {
                        colorIndicator.color = colorDialog.selectedColor;
                        keyboardColorControl.setColor(colorDialog.selectedColor);
                    }
                }
            }
        }
    }

    KeyboardColorControl {
        id: keyboardColorControl
    }

}
