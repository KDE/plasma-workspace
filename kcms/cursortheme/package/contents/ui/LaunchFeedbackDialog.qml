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
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            text: i18nc("@info:usagetip", "Configure the animations played while an application is launching.")
            wrapMode: Text.Wrap
        }

        Kirigami.FormLayout {
            id: formLayout

            readonly property bool cursorImmutable: kcm.launchFeedbackSettings.isBusyCursorImmutable || kcm.launchFeedbackSettings.isBlinkingImmutable || kcm.launchFeedbackSettings.isBouncingImmutable

            QQC2.ButtonGroup {
                id: busyCursorGroup
                onCheckedButtonChanged: {
                    kcm.launchFeedbackSettings.busyCursor = busyCursorStatic.checked || busyCursorBlinking.checked || busyCursorBouncing.checked;
                    kcm.launchFeedbackSettings.blinking = busyCursorBlinking.checked;
                    kcm.launchFeedbackSettings.bouncing = busyCursorBouncing.checked;
                }
            }

            QQC2.RadioButton {
                id: busyCursorDisabled

                Kirigami.FormData.label: i18nc("@label", "Cursor feedback:")
                text: i18nc("@option:radio No cursor feedback when launching apps", "None")
                checked: !kcm.launchFeedbackSettings.busyCursor && !kcm.launchFeedbackSettings.blinking && !kcm.launchFeedbackSettings.bouncing
                enabled: !formLayout.cursorImmutable
                QQC2.ButtonGroup.group: busyCursorGroup
            }

            QQC2.RadioButton {
                id: busyCursorStatic

                text: i18nc("@option:radio", "Static")
                checked: kcm.launchFeedbackSettings.busyCursor && !busyCursorBlinking.checked && !busyCursorBouncing.checked
                enabled: !formLayout.cursorImmutable
                QQC2.ButtonGroup.group: busyCursorGroup
            }

            QQC2.RadioButton {
                id: busyCursorBlinking

                text: i18nc("@option:radio", "Blinking")
                checked: kcm.launchFeedbackSettings.blinking
                enabled: !formLayout.cursorImmutable
                QQC2.ButtonGroup.group: busyCursorGroup
            }

            QQC2.RadioButton {
                id: busyCursorBouncing

                text: i18nc("@option:radio", "Bouncing")
                checked: kcm.launchFeedbackSettings.bouncing
                enabled: !formLayout.cursorImmutable
                QQC2.ButtonGroup.group: busyCursorGroup

                KCM.SettingStateBinding {
                    configObject: kcm.launchFeedbackSettings
                    settingName: "bouncing"
                    extraEnabledConditions: !formLayout.cursorImmutable
                }
            }

            QQC2.CheckBox {
                id: taskManagerNotification

                Kirigami.FormData.label: i18nc("@label", "Task Manager feedback:")

                text: i18nc("@option:check", "Enable animation")

                checked: kcm.launchFeedbackSettings.taskbarButton
                enabled: !kcm.launchFeedbackSettings.isTaskbarButtonImmutable
                onToggled: kcm.launchFeedbackSettings.taskbarButton = checked;

                KCM.SettingStateBinding {
                    configObject: kcm.launchFeedbackSettings
                    settingName: "taskbarButton"
                }
            }

            Item {
                Kirigami.FormData.isSection: true
            }

            QQC2.SpinBox {
                id: notificationTimeout
                Layout.preferredWidth: notificationTimeoutMetrics.implicitWidth + leftPadding + rightPadding
                Kirigami.FormData.label: i18nc("@label", "Stop animations after:")

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
