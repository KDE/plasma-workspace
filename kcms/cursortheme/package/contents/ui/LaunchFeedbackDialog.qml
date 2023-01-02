/*
    SPDX-FileCopyrightText: 2017 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kcm 1.3 as KCM
import org.kde.kirigami 2.20 as Kirigami

Kirigami.OverlaySheet {
    id: sheet

    title: i18nc("@title", "Launch Feedback")

    contentItem: ColumnLayout {
        Kirigami.FormLayout {
            id: formLayout

            QQC2.CheckBox {
                id: busyCursorBouncing

                Kirigami.FormData.label: i18nc("@label", "Cursor:")
                text: i18nc("@option:check", "Enable bouncing feedback")
                checked: kcm.launchFeedbackSettings.busyCursor && !kcm.launchFeedbackSettings.blinking && kcm.launchFeedbackSettings.bouncing
                enabled: !kcm.launchFeedbackSettings.isBusyCursorImmutable && !kcm.launchFeedbackSettings.isBouncingImmutable && !kcm.launchFeedbackSettings.isBlinkingImmutable

                onToggled: {
                    kcm.launchFeedbackSettings.busyCursor = kcm.launchFeedbackSettings.bouncing = busyCursorBouncing.checked;
                    kcm.launchFeedbackSettings.blinking = false; // TODO Plasma 6 Remove
                }

                KCM.SettingStateBinding {
                    configObject: kcm.launchFeedbackSettings
                    settingName: "bouncing"
                    extraEnabledConditions: !busyCursorBouncing.enabled
                }
            }

            Item {
                Kirigami.FormData.isSection: true
            }

            QQC2.CheckBox {
                id: taskManagerNotification

                Kirigami.FormData.label: i18nc("@label", "Task Manager:")

                text: i18nc("@option:check", "Enable animation")

                checked: kcm.launchFeedbackSettings.taskbarButton
                enabled: !kcm.launchFeedbackSettings.isTaskbarButtonImmutable
                onToggled: kcm.launchFeedbackSettings.taskbarButton = checked;

                KCM.SettingStateBinding {
                    configObject: kcm.launchFeedbackSettings
                    settingName: "taskbarButton"
                }
            }

            QQC2.SpinBox {
                id: notificationTimeout
                Layout.preferredWidth: notificationTimeoutMetrics.implicitWidth + leftPadding + rightPadding
                Kirigami.FormData.label: i18nc("@label", "Stop animation after:")

                enabled: !kcm.launchFeedbackSettings.isCursorTimeoutImmutable || !kcm.launchFeedbackSettings.isTaskbarTimeoutImmutable
                from: 1
                to: 60
                stepSize: 1
                editable: true

                value: kcm.launchFeedbackSettings.cursorTimeout
                onValueModified: {
                    kcm.launchFeedbackSettings.cursorTimeout = value
                    kcm.launchFeedbackSettings.taskbarTimeout = value
                }

                textFromValue: function(value, locale) { return i18np("%1 second", "%1 seconds", value)}
                valueFromText: function(text, locale) { return parseInt(text) }

                KCM.SettingStateBinding {
                    configObject: kcm.launchFeedbackSettings
                    settingName: "cursorTimeout"
                    extraEnabledConditions: taskManagerNotification.checked
                }

                TextMetrics {
                    id: notificationTimeoutMetrics
                    font: notificationTimeout.font
                    text: i18np("%1 second", "%1 seconds", 60)
                }
            }
        }
    }

    onSheetOpenChanged: if (!sheetOpen) {
        destroy(Kirigami.Units.humanMoment);
    }

    Component.onCompleted: open();
}
