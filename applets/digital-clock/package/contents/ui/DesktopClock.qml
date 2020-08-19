/*
 * Copyright 2020 Carson Black <uhhadd@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.10
import QtQuick.Layouts 1.10

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PC3

GridLayout {
    anchors.fill: parent

    columns: 2

    states: [
        State {
            name: "allVisible"
            when: digitalClock.showDate && digitalClock.showTimezone
            PropertyChanges {
                target: dateLabel
                visible: true
            }
            PropertyChanges {
                target: tzLabel
                visible: true
            }
        },
        State {
            name: "timezoneHidden"
            when: digitalClock.showDate && !digitalClock.showTimezone
            PropertyChanges {
                target: timeLabel
                horizontalAlignment: Text.AlignHCenter
                Layout.rightMargin: 0
                Layout.columnSpan: 2
            }
            PropertyChanges {
                target: tzLabel
                visible: false
            }
        },
        State {
            name: "dateHidden"
            when: !digitalClock.showDate && digitalClock.showTimezone
            PropertyChanges {
                target: dateLabel
                visible: false
            }
            PropertyChanges {
                target: tzLabel
                visible: true
            }
        },
        State {
            name: "dateAndTimezoneHidden"
            extend: "timezoneHidden"
            when: !digitalClock.showDate && !digitalClock.showTimezone
            PropertyChanges {
                target: dateLabel
                visible: false
            }
        }
    ]

    ResizingLabel {
        id: timeLabel
        visible: true

        text: digitalClock.timeText
        horizontalAlignment: Text.AlignRight

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.rightMargin: PlasmaCore.Units.smallSpacing
        Layout.preferredHeight: 3
        Layout.preferredWidth: 1
    }
    ResizingLabel {
        id: tzLabel
        visible: true

        text: `(${digitalClock.tzText})`
        horizontalAlignment: Text.AlignLeft

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: PlasmaCore.Units.smallSpacing
        Layout.preferredHeight: 3
        Layout.preferredWidth: 1
    }

    ResizingLabel {
        id: dateLabel
        visible: true

        text: digitalClock.dateText
        horizontalAlignment: Text.AlignHCenter

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: 2
        Layout.columnSpan: 2
    }
}