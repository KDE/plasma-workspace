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

    ConfigModule.buttons: ConfigModule.Defaults | ConfigModule.Apply
    leftPadding: width * 0.1
    rightPadding: leftPadding

    Kirigami.FormLayout {
        QQC2.CheckBox {
            Kirigami.FormData.label: i18n("Feedback:")
            checked: kcm.feedbackEnabled
            onToggled: kcm.feedbackEnabled = checked
            text: i18n("Allow KDE software to collect anomymous usage information")
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        QQC2.Label {
            Kirigami.FormData.label: i18n("Plasma:")
            Layout.maximumWidth: root.width * 0.5
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: xi18nc("@info", "We make Plasma for you. You can help us improve it by contributing information on how you use it. This allows us to focus on things that matter to you.<nl/><nl/>Contributing usage information is optional and entirely anonymous. It will not associate the data with any kind of unique identifier, and will never track the documents you open, the websites you visit, or any other kind of personal information.<nl/><nl/>You can read more about our policy in the following link.")
        }

        Kirigami.UrlButton {
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.gridUnit
            url: "https://kde.org/privacypolicy-apps.php"
        }

        QQC2.ComboBox {
            id: statisticsModeCombo
            enabled: kcm.feedbackEnabled
            Layout.fillWidth: true
            textRole: "text"
            model: ListModel { id: modeOptions }

            Component.onCompleted: {
                modeOptions.append({text: i18n("Basic System Information"), value: UserFeedback.Provider.BasicSystemInformation})
                modeOptions.append({text: i18n("Basic Usage Information"), value: UserFeedback.Provider.BasicUsageStatistics})
                modeOptions.append({text: i18n("Detailed System Information"), value: UserFeedback.Provider.DetailedSystemInformation})
                modeOptions.append({text: i18n("Detailed Usage Information"), value: UserFeedback.Provider.DetailedUsageStatistics})

                for(var i = 0, c=modeOptions.count; i<c; ++i) {
                    if (modeOptions.get(i).value === kcm.plasmaFeedbackLevel) {
                        currentIndex = i;
                        break;
                    }
                }
                if (currentIndex < 0)
                    currentIndex = 2
            }

            onActivated: {
                kcm.plasmaFeedbackLevel = modeOptions.get(index).value;
            }
        }

        UserFeedback.FeedbackConfigUiController {
            id: feedbackController
        }

        QQC2.Label {
            Layout.maximumWidth: root.width * 0.5
            wrapMode: Text.WordWrap
            text: feedbackController.telemetryDescription(modeOptions.get(statisticsModeCombo.currentIndex).value)
        }
    }
}

