/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QQC2
import org.kde.kirigami 2.6 as Kirigami
import org.kde.userfeedback 1.0 as UserFeedback
import org.kde.userfeedback.kcm 1.0
import org.kde.kcm 1.3

SimpleKCM {
    id: root

    ConfigModule.buttons: ConfigModule.Default | ConfigModule.Apply

    implicitWidth: Kirigami.Units.gridUnit * 38
    implicitHeight: Kirigami.Units.gridUnit * 35


    ColumnLayout {
        spacing: 0

        Kirigami.InlineMessage {
            id: infoLabel
            Layout.fillWidth: true

            type: Kirigami.MessageType.Information
            visible: !form.enabled
            text: i18n("User Feedback has been disabled centrally. Please contact your distributor.")
        }

        // The system settings window likes to take over
        // the cursor with a plain label. The TextEdit
        // 'takes priority' over the system settings
        // window trying to eat the mouse, allowing
        // us to use the HoverHandler boilerplate for
        // proper link handling
        TextEdit {
            Kirigami.FormData.label: i18n("Plasma:")
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.gridUnit
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.gridUnit
            Layout.alignment: Qt.AlignHCenter
            wrapMode: Text.WordWrap
            text: xi18nc("@info", "You can help KDE improve Plasma by contributing information on how you use it, so we can focus on things that matter to you.<nl/><nl/>Contributing this information is optional and entirely anonymous. We never collect your personal data, files you use, websites you visit, or information that could identify you.<nl/><nl/>You can read about <link url='https://kde.org/privacypolicy-apps.php'>our privacy policy here.</link>")
            textFormat: TextEdit.RichText
            readOnly: true

            color: Kirigami.Theme.textColor
            selectedTextColor: Kirigami.Theme.highlightedTextColor
            selectionColor: Kirigami.Theme.highlightColor

            onLinkActivated: (url) => Qt.openUrlExternally(url)

            HoverHandler {
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.gridUnit
        }

        Kirigami.FormLayout {
            id: form
            enabled: kcm.feedbackEnabled
            QQC2.Slider {
                id: statisticsModeSlider
                Kirigami.FormData.label: i18n("Plasma:")
                readonly property var currentMode: modeOptions[value]
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 22

                readonly property var modeOptions: [UserFeedback.Provider.NoTelemetry, UserFeedback.Provider.BasicSystemInformation, UserFeedback.Provider.BasicUsageStatistics,
                                                    UserFeedback.Provider.DetailedSystemInformation, UserFeedback.Provider.DetailedUsageStatistics]
                from: 0
                to: modeOptions.length - 1
                stepSize: 1
                snapMode: QQC2.Slider.SnapAlways

                function findIndex(array, what, defaultValue) {
                    for (var v in array) {
                        if (array[v] == what)
                            return v;
                    }
                    return defaultValue;
                }

                value: findIndex(modeOptions, kcm.feedbackSettings.feedbackLevel, 0)

                onMoved: {
                    kcm.feedbackSettings.feedbackLevel = modeOptions[value]
                }

                SettingStateBinding {
                    configObject: kcm.feedbackSettings
                    settingName: "feedbackLevel"
                    extraEnabledConditions: kcm.feedbackEnabled
                }
            }

            UserFeedback.FeedbackConfigUiController {
                id: feedbackController
                applicationName: i18n("Plasma")
            }

            Kirigami.Heading {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Kirigami.Units.gridUnit * 22
                Layout.maximumWidth: Kirigami.Units.gridUnit * 29
                wrapMode: Text.WordWrap
                level: 3
                text: feedbackController.telemetryName(statisticsModeSlider.currentMode)
            }
            Item {
                Kirigami.FormData.isSection: true
            }
            QQC2.Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: Kirigami.Units.gridUnit * 22
                wrapMode: Text.WordWrap

                text: i18n("The following information will be sent:")
                visible: statisticsModeSlider.value != 0 // This is "disabled"
            }
            ColumnLayout {
                Layout.maximumWidth: parent.width * 0.5
                Repeater {
                    model: kcm.feedbackSources
                    delegate: QQC2.Label {
                        visible: modelData.mode <= statisticsModeSlider.currentMode
                        text: "· " + modelData.description
                        Layout.fillWidth: true

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            QQC2.ToolTip {
                                width: iconsLayout.implicitWidth + Kirigami.Units.largeSpacing * 2
                                height: iconsLayout.implicitHeight + Kirigami.Units.smallSpacing * 2
                                visible: parent.containsMouse
                                RowLayout {
                                    id: iconsLayout
                                    Repeater {
                                        model: modelData.icons
                                        delegate: Kirigami.Icon {
                                            implicitHeight: Kirigami.Units.iconSizes.medium
                                            implicitWidth: Kirigami.Units.iconSizes.medium
                                            source: modelData
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item {
                implicitHeight: Kirigami.Units.largeSpacing
                Layout.fillWidth: true
            }
            Repeater {
                model: kcm.audits
                delegate: Kirigami.LinkButton {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    Kirigami.FormData.label: model.index === 0 ? i18n("View sent data:") : ""
                    text: modelData.program
                    onClicked: Qt.openUrlExternally(modelData.audits)
                }
            }
        }
    }
}

