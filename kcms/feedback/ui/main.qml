/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.userfeedback as UserFeedback
import org.kde.userfeedback.kcm
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: root

    KCM.ConfigModule.buttons: KCM.ConfigModule.Default | KCM.ConfigModule.Apply

    headerPaddingEnabled: false // Let the InlineMessage touch the edges
    header: Kirigami.InlineMessage {
        id: infoLabel

        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Information
        visible: !form.enabled
        text: i18n("User Feedback has been disabled centrally. Please contact your distributor.")
    }

    UserFeedback.FeedbackConfigUiController {
        id: feedbackController
        applicationName: i18n("Plasma")
    }

    ColumnLayout {
    QQC2.Label {
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit
        Layout.leftMargin: Kirigami.Units.gridUnit
        Layout.rightMargin: Kirigami.Units.gridUnit
        Layout.maximumWidth: form.implicitWidth
        Layout.alignment: Qt.AlignCenter

        text: i18nc("@info", "Help KDE improve Plasma by sending system and usage information. This is optional and entirely anonymous—we never collect your personal data, files you use, websites you visit, or information that could identify you.")
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }

    Kirigami.UrlButton {
        Layout.alignment: Qt.AlignHCenter
        Layout.bottomMargin: Kirigami.Units.gridUnit
        url: "https://kde.org/privacypolicy-apps.php"
        text: i18nc("@info", "Read more about our privacy policy")
    }

    Kirigami.Form {
        id: form
        Layout.fillWidth: true
        enabled: kcm.feedbackEnabled

        Kirigami.FormGroup {
            Kirigami.FormEntry {
                title: i18n("Plasma:")

                contentItem: QQC2.Slider {
                    id: statisticsModeSlider
                    Layout.fillWidth: true

                    readonly property var currentMode: modeOptions[value]
                    readonly property var modeOptions: [
                        UserFeedback.Provider.NoTelemetry,
                        UserFeedback.Provider.BasicSystemInformation,
                        UserFeedback.Provider.BasicUsageStatistics,
                        UserFeedback.Provider.DetailedSystemInformation,
                        UserFeedback.Provider.DetailedUsageStatistics
                    ]

                    from: 0
                    to: modeOptions.length - 1
                    stepSize: 1
                    snapMode: QQC2.Slider.SnapAlways

                    Kirigami.StyleHints.tickMarkStepSize: 1

                    value: {
                        const index = modeOptions.indexOf(kcm.feedbackSettings.feedbackLevel);
                        return index === -1 ? 0 : index;
                    }

                    onMoved: {
                        kcm.feedbackSettings.feedbackLevel = modeOptions[value]
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.feedbackSettings
                        settingName: "feedbackLevel"
                        extraEnabledConditions: kcm.feedbackEnabled
                    }
                }
            }

            Kirigami.FormEntry {
                contentItem: QQC2.Label {
                    text: feedbackController.telemetryName(statisticsModeSlider.currentMode)
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }
            }
        }

        Kirigami.FormGroup {
            visible: statisticsModeSlider.value !== 0 // This is "disabled"

            Kirigami.FormEntry {
                contentItem: QQC2.Label {
                    text: i18n("The following information will be sent:")
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }
            }

            Repeater {
                model: kcm.feedbackSources
                delegate: Kirigami.FormEntry {
                    visible: modelData.mode <= statisticsModeSlider.currentMode
                    contentItem: QQC2.Label {
                        text: "· " + modelData.description
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        Kirigami.FormGroup {
            visible: kcm.audits.length > 0
            Repeater {
                model: kcm.audits
                delegate: Kirigami.FormEntry {
                    title: index === 0 ? i18n("View sent data:") : ""
                    contentItem: Kirigami.LinkButton {
                        horizontalAlignment: Text.AlignLeft
                        text: modelData.program
                        onClicked: Qt.openUrlExternally("file://" + modelData.audits)
                    }
                }
            }
        }
    }
    }
}
