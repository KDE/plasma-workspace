/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015, 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

Item {
    id: daysCalendar

    signal activated(int index, var date, var item)

    readonly property int gridColumns: showWeekNumbers ? calendarGrid.columns + 1 : calendarGrid.columns

    property int rows
    property int columns

    property bool showWeekNumbers

    // how precise date matching should be, 3 = day+month+year, 2 = month+year, 1 = just year
    property int dateMatchingPrecision

    property alias repeater: repeater
    property alias headerModel: days.model
    property alias gridModel: repeater.model

    // Take the calendar width, subtract the inner and outer spacings and divide by number of columns (==days in week)
    readonly property int cellWidth: Math.floor((swipeView.width - (daysCalendar.columns + 1) * root.borderWidth) / (daysCalendar.columns + (showWeekNumbers ? 1 : 0)))
    // Take the calendar height, subtract the inner spacings and divide by number of rows (root.weeks + one row for day names)
    readonly property int cellHeight:  Math.floor((swipeView.height - viewHeader.heading.height - calendarGrid.rows * root.borderWidth) / calendarGrid.rows)

    Column {
        id: weeksColumn
        visible: showWeekNumbers
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            // The borderWidth needs to be counted twice here because it goes
            // in fact through two lines - the topmost one (the outer edge)
            // and then the one below weekday strings
            topMargin: daysCalendar.cellHeight + root.borderWidth + root.borderWidth
        }
        spacing: root.borderWidth

        Repeater {
            model: showWeekNumbers ? calendarBackend.weeksModel : []

            PlasmaComponents.Label {
                height: daysCalendar.cellHeight
                width: daysCalendar.cellWidth
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                opacity: 0.4
                text: modelData
                textFormat: Text.PlainText
                font.pixelSize: Math.max(Kirigami.Theme.smallFont.pixelSize, daysCalendar.cellHeight / 3)
            }
        }
    }

    Grid {
        id: calendarGrid

        anchors {
            top: parent.top
            right: parent.right
            rightMargin: root.borderWidth
            bottom: parent.bottom
            bottomMargin: root.borderWidth
        }

        columns: daysCalendar.columns
        rows: daysCalendar.rows + (daysCalendar.headerModel ? 1 : 0)

        spacing: root.borderWidth
        columnSpacing: parent.squareCell ? (daysCalendar.width - daysCalendar.columns * (daysCalendar.cellWidth - root.borderWidth)) / daysCalendar.columns : root.borderWidth

        Repeater {
            id: days

            PlasmaComponents.Label {
                width: daysCalendar.cellWidth
                height: daysCalendar.cellHeight
                text: Qt.locale(Qt.locale().uiLanguages[0]).dayName(((calendarBackend.firstDayOfWeek + index) % days.count), Locale.ShortFormat)
                textFormat: Text.PlainText
                font.pixelSize: Math.max(Kirigami.Theme.smallFont.pixelSize, daysCalendar.cellHeight / 3)
                opacity: 0.4
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                fontSizeMode: Text.HorizontalFit
            }
        }

        Repeater {
            id: repeater

            DayDelegate {
                id: delegate
                width: daysCalendar.cellWidth
                height: daysCalendar.cellHeight
                dayModel: repeater.model

                Keys.onPressed: event => {
                    if (!daysCalendar.PlasmaComponents.SwipeView.isCurrentItem) {
                        event.accepted = false;
                        return;
                    }
                    switch (event.key) {
                    case Qt.Key_Space:
                    case Qt.Key_Enter:
                    case Qt.Key_Return:
                    case Qt.Key_Select:
                        daysCalendar.activated(index, model, delegate);
                        break;
                    }
                }

                KeyNavigation.left: if (index !== 0) {
                    return repeater.itemAt(index - 1);
                } else {
                    return daysCalendar.KeyNavigation.left;
                }
                KeyNavigation.tab: daysCalendar.KeyNavigation.tab

                Keys.onUpPressed: event => {
                    if (index >= daysCalendar.columns) {
                        repeater.itemAt(index - daysCalendar.columns).forceActiveFocus(Qt.TabFocusReason);
                    } else {
                        event.accepted = false;
                    }
                }
                Keys.onDownPressed: event => {
                    if (index < (daysCalendar.rows - 1) * daysCalendar.columns) {
                        repeater.itemAt(index + daysCalendar.columns).forceActiveFocus(Qt.TabFocusReason);
                    }
                }

                onClicked: {
                    daysCalendar.activated(index, model, delegate)
                }
            }
        }
    }
}
