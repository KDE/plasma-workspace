/*
 * Copyright (C) 2019 David Edmundson <davidedmundson@kde.org>
 * Copyright (C) 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QQC2
import org.kde.kirigami 2.6 as Kirigami
import org.kde.userfeedback 1.0 as UserFeedback
import org.kde.kcm 1.2

SimpleKCM {
    id: root

    ConfigModule.buttons: ConfigModule.Default | ConfigModule.Apply
    leftPadding: width * 0.1
    rightPadding: leftPadding

    implicitHeight: Kirigami.Units.gridUnit * 23


    ColumnLayout {
        Kirigami.InlineMessage {
            id: infoLabel
            Layout.fillWidth: true

            type: Kirigami.MessageType.Information
            visible: !form.enabled
            text: i18n("User Feedback has been disabled centrally. Please contact your distributor.")
        }

        QQC2.Label {
            Kirigami.FormData.label: i18n("Plasma:")
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: xi18nc("@info", "You can help KDE improve Plasma by contributing information on how you use it, so we can focus on things that matter to you.<nl/><nl/>Contributing this information is optional and entirely anonymous. We never collect your personal data, files you use, websites you visit, or information that could identify you.<nl/><nl/>You can read about our policy in the following link:")
        }

        Kirigami.UrlButton {
            Layout.alignment: Qt.AlignHCenter
            url: "https://kde.org/privacypolicy-apps.php"
        }

        Kirigami.Separator {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.gridUnit
        }

        Kirigami.FormLayout {
            id: form
            enabled: kcm.feedbackEnabled
            QQC2.Slider {
                id: statisticsModeSlider
                Kirigami.FormData.label: i18n("Plasma:")
                readonly property var currentMode: modeOptions[value]
                enabled: kcm.feedbackEnabled
                Layout.fillWidth: true
                Layout.minimumWidth: Kirigami.Units.gridUnit * 21

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

                value: findIndex(modeOptions, kcm.plasmaFeedbackLevel, 0)

                onMoved: {
                    kcm.plasmaFeedbackLevel = modeOptions[value]
                }
            }

            UserFeedback.FeedbackConfigUiController {
                id: feedbackController
                applicationName: i18n("Plasma")
            }

            Kirigami.Heading {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: statisticsModeSlider.width
                wrapMode: Text.WordWrap
                level: 3
                text: feedbackController.telemetryName(statisticsModeSlider.currentMode)
            }
            QQC2.Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: statisticsModeSlider.width
                wrapMode: Text.WordWrap

                text: {
                    feedbackController.applicationName
                    return feedbackController.telemetryDescription(statisticsModeSlider.currentMode)
                }
            }
            ColumnLayout {
                Layout.maximumWidth: parent.width * 0.5
                Repeater {
                    model: kcm.feedbackSources
                    delegate: QQC2.Label {
                        visible: modelData.mode <= statisticsModeSlider.currentMode
                        text: "· " + modelData.description

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            QQC2.ToolTip {
                                width: iconsLayout.implicitWidth + Kirigami.Units.largeSpacing * 2
                                height: iconsLayout.implicitHeight + Kirigami.Units.smallSpacing * 2
                                visible: parent.containsMouse
                                RowLayout {
                                    id: iconsLayout
                                    anchors.centerIn: parent
                                    Repeater {
                                        model: modelData.icons
                                        delegate: Image {
                                            source: "image://icon/" + modelData
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                opacity: 0.6
            }
        }
    }
}

