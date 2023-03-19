/*
    SPDX-FileCopyrightText: 2025 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/


import QtQuick 2.15
import QtQml 2.15
import org.kde.plasma.clock 1.0

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
                }
                Text {
                    text: clock.timeZoneName
                }
                Text {
                    text: Qt.formatDateTime(clock.dateTime, Qt.locale().dateFormat(Locale.ShortFormat))
                }
                Text {
                    text: Qt.formatDateTime(clock.dateTime, Qt.locale().timeFormat(Locale.ShortFormat))
                }
            }
        }
    }
}
