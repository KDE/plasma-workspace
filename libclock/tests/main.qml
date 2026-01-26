/*
    SPDX-FileCopyrightText: 2025 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/


import QtQuick
import QtQml
import org.kde.plasma.clock

Item {
    width: 500
    height: 500

    ListModel {
        id: tzModel
        ListElement {
            timeZone: null
        }
        ListElement {
            timeZone: "Europe/Berlin"
        }
        ListElement {
            timeZone: "US/Pacific"
        }
    }

    Column {
        Repeater {
            model: tzModel
            Row {
                id: delegate
                spacing: 10
                required property string timeZone
                Clock {
                    id: clock
                    timeZone: delegate.timeZone
                    trackSeconds: true
                }
                Text {
                    text: (timeZone || "System")
                    textFormat: Text.PlainText
                }
                Text {
                    text: clock.timeZoneName
                    textFormat: Text.PlainText
                }
                Text {
                    text: Qt.formatDateTime(clock.dateTime, Qt.locale().dateFormat(Locale.ShortFormat))
                    textFormat: Text.PlainText
                }
                Text {
                    text: Qt.formatDateTime(clock.dateTime, Qt.locale().timeFormat(Locale.ShortFormat))
                    textFormat: Text.PlainText
                }
            }
        }
    }
}
