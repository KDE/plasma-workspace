/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls
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
            spacing: Kirigami.Units.gridUnit * 0.5

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

                ColumnLayout {
                    Layout.alignment: Qt.AlignLeft
                    Layout.leftMargin: Math.max(Math.round(0.5 * (sunPathChart.width - width) - sunPathChart.daylightSpan), Kirigami.Units.gridUnit)
                    ToolTip.visible: sunriseHoverHandler.hovered
                    ToolTip.text: i18n("Sunrise starts at %1 and ends at %2", formatTime(schedule.startSunriseDateTime), formatTime(schedule.endSunriseDateTime))

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: i18n("Sunrise")
                        textFormat: Text.PlainText
                        font.bold: true
                    }

                    Label {
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
                    ToolTip.visible: sunsetHoverHandler.hovered
                    ToolTip.text: i18n("Sunset starts at %1 and ends at %2", formatTime(schedule.startSunsetDateTime), formatTime(schedule.endSunsetDateTime))

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: i18n("Sunset")
                        textFormat: Text.PlainText
                        font.bold: true
                    }

                    Label {
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
                return i18n("Sunrise and sunset times cannot be determined at your location due to polar day. Using custom times instead.");
            } else if (schedule.fallbackReason === Private.DarkLightSchedulePreview.FallbackReason.PolarNight) {
                return i18n("Sunrise and sunset times cannot be determined at your location due to polar night. Using custom times instead.");
            } else {
                return i18n("Sunrise and sunset times cannot be reliably determined at your location. Using custom times instead.");
            }
        }
    }

    function formatTime(dateTime: date): string {
        return dateTime.toLocaleTimeString(Qt.locale(), Locale.ShortFormat);
    }
}
