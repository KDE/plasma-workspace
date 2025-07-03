/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.private.kcms.nighttime as Private

ColumnLayout {
    spacing: Kirigami.Units.smallSpacing

    required property Private.DarkLightSchedulePreview schedule

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: sunChartColumn.implicitHeight

        Private.DashedBackground {
            anchors.fill: parent
            visible: schedule.fallbackReason !== Private.DarkLightSchedulePreview.FallbackReason.None
            color: Kirigami.Theme.negativeTextColor
        }

        ColumnLayout {
            id: sunChartColumn
            anchors.fill: parent
            spacing: Kirigami.Units.largeSpacing

            Private.SunPathChart {
                id: sunPathChart
                Layout.fillWidth: true
                Layout.minimumHeight: Kirigami.Units.gridUnit * 5
                sunriseDateTime: schedule.endSunriseDateTime
                sunsetDateTime: schedule.startSunsetDateTime
                dayColor: Kirigami.Theme.neutralTextColor
                nightColor: Kirigami.Theme.disabledTextColor
            }

            RowLayout {
                Layout.preferredWidth: sunPathChart.width
                spacing: Kirigami.Units.smallSpacing

                ColumnLayout {
                    Layout.alignment: Qt.AlignLeft
                    Layout.leftMargin: Math.max(Math.round(0.5 * (sunPathChart.width - width) - sunPathChart.daylightSpan), Kirigami.Units.gridUnit)
                    QQC2.ToolTip.visible: sunriseHoverHandler.hovered
                    QQC2.ToolTip.text: i18nc("@info:tooltip", "Sunrise starts at %1 and ends at %2", formatTime(schedule.startSunriseDateTime), formatTime(schedule.endSunriseDateTime))
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: i18nc("@label", "Sunrise")
                        textFormat: Text.PlainText
                        font.bold: true
                    }

                    QQC2.Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: formatTime(schedule.endSunriseDateTime)
                        textFormat: Text.PlainText
                    }

                    HoverHandler {
                        id: sunriseHoverHandler
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: Math.max(Math.round(0.5 * (sunPathChart.width - width) - sunPathChart.daylightSpan), Kirigami.Units.gridUnit)
                    QQC2.ToolTip.visible: sunsetHoverHandler.hovered
                    QQC2.ToolTip.text: i18nc("@info:tooltip", "Sunset starts at %1 and ends at %2", formatTime(schedule.startSunsetDateTime), formatTime(schedule.endSunsetDateTime))
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: i18nc("@label", "Sunset")
                        textFormat: Text.PlainText
                        font.bold: true
                    }

                    QQC2.Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: formatTime(schedule.startSunsetDateTime)
                        textFormat: Text.PlainText
                    }

                    HoverHandler {
                        id: sunsetHoverHandler
                    }
                }
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        type: Kirigami.MessageType.Warning
        visible: schedule.fallbackReason !== Private.DarkLightSchedulePreview.FallbackReason.None
        text: {
            if (schedule.fallbackReason === Private.DarkLightSchedulePreview.FallbackReason.PolarDay) {
                return i18nc("@info", "Sunrise and sunset times cannot be determined at your location due to polar day. Choose custom times instead.");
            } else if (schedule.fallbackReason === Private.DarkLightSchedulePreview.FallbackReason.PolarNight) {
                return i18nc("@info", "Sunrise and sunset times cannot be determined at your location due to polar night. Choose custom times instead.");
            } else {
                return i18nc("@info", "Sunrise and sunset times cannot be reliably determined at your location. Choose custom times instead.");
            }
        }
    }

    function formatTime(dateTime: date): string {
        return dateTime.toLocaleTimeString(Qt.locale(), Locale.ShortFormat);
    }
}
