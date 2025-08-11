/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2025 Akseli Lahtinen <akselmo@akselmo.dev>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

import org.kde.notificationmanager as NotificationManager

Kirigami.Dialog {
    id: positionPopup

    // Only save the screen selection when the user changed it to avoid changing it
    // when the previously selected screen isn't currently available.
    property bool screenDirty: false

    readonly property bool invalidConfiguration: screenCombo.currentIndex > 0 && positionSelector.selectedPosition === NotificationManager.Settings.CloseToWidget

    title: i18n("Popup Position")
    showCloseButton: false
    padding: Kirigami.Units.largeSpacing

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.FormLayout {
            Layout.fillWidth: true

            QtControls.ComboBox {
                id: screenCombo

                Kirigami.FormData.label: i18n("Screen:")
                model: {
                    let model = Application.screens.map((screen, index) => i18nc("Screen number (name)", "Screen %1 (%2)", index + 1, screen.name));
                    model.unshift(i18n("Automatic"));
                    return model;
                }
                currentIndex: {
                    let index = kcm.notificationSettings.popupScreen + 1;
                    // When selected screen isn't available, use primary screen.
                    // Do not switch to "Auto" since Auto will always be there, so it couldn't been.
                    if (index >= count) {
                        index = 1;
                    }
                    return index;
                }
                //visible: Application.screens.length > 1 || kcm.notificationSettings.popupScreen > -1
                onActivated: {
                    positionPopup.screenDirty = true;
                }

                // FIXME How do I use this here? Since our index is off-by-one.
                KCM.SettingStateBinding {
                    configObject: kcm.notificationSettings
                    settingName: "PopupPosition"
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            text: i18n("Placing notifications near the notification icon is not possible when a custom screen is assigned. Please choose a screen corner below.")
            visible: positionPopup.invalidConfiguration
        }

        ScreenPositionSelector {
            id: positionSelector
            Layout.alignment: Qt.AlignHCenter
            selectedPosition: kcm.notificationSettings.popupPosition
        }
    }

    footer: QtControls.DialogButtonBox {
        standardButtons: QtControls.DialogButtonBox.Ok | QtControls.DialogButtonBox.Cancel
        onAccepted: {
            kcm.notificationSettings.popupPosition = positionSelector.selectedPosition;
            if (positionPopup.screenDirty) {
                kcm.notificationSettings.popupScreen = screenCombo.currentIndex - 1;
            }
        }

        Component.onCompleted: {
            const okButton = standardButton(QtControls.DialogButtonBox.Ok);
            okButton.enabled = Qt.binding(() => {
                return !positionPopup.invalidConfiguration;
            });
        }
    }
}
