/*
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 * Copyright 2015 Martin Klapetek <mklapetek@kde.org>
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
import QtQuick 2.4
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.calendar 2.0 as PlasmaCalendar
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

PlasmaComponents3.Page {
    id: calendar

    // The "sensible" values
    property int _minimumWidth: rootLayout.childrenRect.width + (calendar.paddings * 2)
    property int _minimumHeight: rootLayout.childrenRect.height + (calendar.paddings * 2) + headerArea.height

    Layout.minimumWidth: _minimumWidth
    Layout.minimumHeight: _minimumHeight
    Layout.preferredWidth: _minimumWidth
    Layout.preferredHeight: _minimumHeight
    Layout.maximumWidth: _minimumWidth
    Layout.maximumHeight: _minimumHeight

    readonly property int paddings: units.smallSpacing
    readonly property bool showAgenda: PlasmaCalendar.EventPluginsManager.enabledPlugins.length > 0
    readonly property bool showClocks: plasmoid.configuration.selectedTimeZones.length > 1

    property alias borderWidth: monthView.borderWidth
    property alias monthView: monthView

    property bool debug: false

    property bool isExpanded: plasmoid.expanded

    onIsExpandedChanged: {
        // clear all the selections when the plasmoid is showing/hiding
        monthView.resetToToday();
    }

    // Header containing date and pin button
    header: PlasmaExtras.PlasmoidHeading {
        id: headerArea

        RowLayout {
            width: parent.width

            PlasmaExtras.Heading {
                Layout.fillWidth: true
                Layout.leftMargin: calendar.paddings // Match calendar title

                level: 1

                text: monthView.currentDate.toLocaleDateString(Qt.locale(), Locale.LongFormat)
            }

            PlasmaComponents3.ToolButton {
                visible: plasmoid.action("configure").enabled
                icon.name: "configure"
                onClicked: plasmoid.action("configure").trigger()
                PlasmaComponents3.ToolTip {
                    text: plasmoid.action("configure").text
                }
            }

            // Allows the user to keep the calendar open for reference
            PlasmaComponents3.ToolButton {
                checkable: true
                checked: plasmoid.configuration.pin
                onToggled: plasmoid.configuration.pin = checked
                icon.name: "window-pin"
                PlasmaComponents3.ToolTip {
                    text: i18n("Keep Open")
                }
            }
        }
    }
    // Top-level layout containing:
    // - Left column with current date header, calendar, and agenda view
    // - Right column with world clocks
    RowLayout {
        id: rootLayout

        anchors {
            top: parent.top
            left: parent.left
            margins: calendar.paddings
        }

        spacing: calendar.paddings

        // Left column containing calendar
        // ===============================
        // TODO KF6: remove the `Item` wrapper, which this is only needed since
        // PlasmaCalendar.MonthView internally has `anchors.fill:parent` set on
        // it, erroneously expecting to never be in a Layout
        Item {
            Layout.fillWidth: true
            Layout.minimumHeight: units.gridUnit * 22
            Layout.minimumWidth: units.gridUnit * 22

            PlasmaCalendar.MonthView {
                id: monthView
                borderOpacity: 0.25
                today: root.tzDate
                showWeekNumbers: plasmoid.configuration.showWeekNumbers
            }
        }


        // Vertical separator line between columns
        // =======================================
        PlasmaCore.SvgItem {
            visible: rightColumn.visible

            Layout.preferredWidth: naturalSize.width
            Layout.fillHeight: true
            // Unify margins because the calendar includes its own
            Layout.topMargin: calendar.paddings
            Layout.rightMargin: calendar.paddings

            elementId: "vertical-line"
            svg: PlasmaCore.Svg {
                imagePath: "widgets/line"
            }
        }


        // Right column containing agenda view and time zones
        // ==================================================
        ColumnLayout {
            id: rightColumn

            visible: agenda.visible || worldClocks.visible

            Layout.minimumWidth: units.gridUnit * 14

            // Agenda view stuff
            // -----------------
            // Header text
            PlasmaExtras.Heading {
                visible: agenda.visible

                Layout.fillWidth: true

                level: 2

                text: i18n("Events")
                maximumLineCount: 1
                elide: Text.ElideRight
            }

            // Agenda view itself
            Item {
                id: agenda
                visible: calendar.showAgenda

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: units.gridUnit * 4
                Layout.leftMargin: -units.smallSpacing

                function formatDateWithoutYear(date) {
                    // Unfortunatelly Qt overrides ECMA's Date.toLocaleDateString(),
                    // which is able to return locale-specific date-and-month-only date
                    // formats, with its dumb version that only supports Qt::DateFormat
                    // enum subset. So to get a day-and-month-only date format string we
                    // must resort to this magic and hope there are no locales that use
                    // other separators...
                    var format = Qt.locale().dateFormat(Locale.ShortFormat).replace(/[./ ]*Y{2,4}[./ ]*/i, '');
                    return Qt.formatDate(date, format);
                }

                function dateEquals(date1, date2) {
                    var values1 = [
                        date1.getFullYear(),
                        date1.getMonth(),
                        date1.getDate()
                    ];

                    var values2 = [
                        date2.getFullYear(),
                        date2.getMonth(),
                        date2.getDate()
                    ];

                    return values1.every((value, index) => {
                        return (value === values2[index]);
                    }, false)
                }

                Connections {
                    target: monthView

                    onCurrentDateChanged: {
                        // Apparently this is needed because this is a simple QList being
                        // returned and if the list for the current day has 1 event and the
                        // user clicks some other date which also has 1 event, QML sees the
                        // sizes match and does not update the labels with the content.
                        // Resetting the model to null first clears it and then correct data
                        // are displayed.
                        holidaysList.model = null;
                        holidaysList.model = monthView.daysModel.eventsForDate(monthView.currentDate);
                    }
                }

                Connections {
                    target: monthView.daysModel

                    onAgendaUpdated: {
                        if (agenda.dateEquals(updatedDate, monthView.currentDate)) {
                            holidaysList.model = null;
                            holidaysList.model = monthView.daysModel.eventsForDate(monthView.currentDate);
                        }
                    }
                }

                Connections {
                    target: plasmoid.configuration

                    onEnabledCalendarPluginsChanged: {
                        PlasmaCalendar.EventPluginsManager.enabledPlugins = plasmoid.configuration.enabledCalendarPlugins;
                    }
                }

                Binding {
                    target: plasmoid
                    property: "hideOnWindowDeactivate"
                    value: !plasmoid.configuration.pin
                }

                TextMetrics {
                    id: dateLabelMetrics

                    // Date/time are arbitrary values with all parts being two-digit
                    readonly property string timeString: Qt.formatTime(new Date(2000, 12, 12, 12, 12, 12, 12))
                    readonly property string dateString: agenda.formatDateWithoutYear(new Date(2000, 12, 12, 12, 12, 12))

                    font: theme.defaultFont
                    text: timeString.length > dateString.length ? timeString : dateString
                }

                PlasmaExtras.ScrollArea {
                    id: holidaysView
                    anchors.fill: parent

                    ListView {
                        id: holidaysList

                        delegate: PlasmaExtras.ListItem {
                            id: eventItem
                            implicitHeight: eventGrid.height + PlasmaCore.Units.smallSpacing * 2
                            property bool hasTime: {
                                // Explicitly all-day event
                                if (modelData.isAllDay) {
                                    return false;
                                }
                                // Multi-day event which does not start or end today (so
                                // is all-day from today's point of view)
                                if (modelData.startDateTime - monthView.currentDate < 0 &&
                                    modelData.endDateTime - monthView.currentDate > 86400000) { // 24hrs in ms
                                    return false;
                                }

                                // Non-explicit all-day event
                                var startIsMidnight = modelData.startDateTime.getHours() === 0
                                                && modelData.startDateTime.getMinutes() === 0;

                                var endIsMidnight = modelData.endDateTime.getHours() === 0
                                                && modelData.endDateTime.getMinutes() === 0;

                                var sameDay = modelData.startDateTime.getDate() === modelData.endDateTime.getDate()
                                        && modelData.startDateTime.getDay() === modelData.endDateTime.getDay()

                                if (startIsMidnight && endIsMidnight && sameDay) {
                                    return false
                                }

                                return true;
                            }

                            PlasmaCore.ToolTipArea {
                                width: parent.width
                                height: eventGrid.height
                                active: eventTitle.truncated || eventDescription.truncated
                                mainText: active ? eventTitle.text : ""
                                subText: active ? eventDescription.text : ""

                                GridLayout {
                                    id: eventGrid
                                    columns: 3
                                    rows: 2
                                    rowSpacing: 0
                                    columnSpacing: 2 * units.smallSpacing

                                    width: parent.width

                                    Rectangle {
                                        id: eventColor

                                        Layout.row: 0
                                        Layout.column: 0
                                        Layout.rowSpan: 2
                                        Layout.fillHeight: true

                                        color: modelData.eventColor
                                        width: 5 * units.devicePixelRatio
                                        visible: modelData.eventColor !== ""
                                    }

                                    PlasmaComponents3.Label {
                                        id: startTimeLabel

                                        readonly property bool startsToday: modelData.startDateTime - monthView.currentDate >= 0
                                        readonly property bool startedYesterdayLessThan12HoursAgo: modelData.startDateTime - monthView.currentDate >= -43200000 //12hrs in ms

                                        Layout.row: 0
                                        Layout.column: 1
                                        Layout.minimumWidth: dateLabelMetrics.width

                                        text: startsToday || startedYesterdayLessThan12HoursAgo
                                                ? Qt.formatTime(modelData.startDateTime)
                                                : agenda.formatDateWithoutYear(modelData.startDateTime)
                                        horizontalAlignment: Qt.AlignRight
                                        visible: eventItem.hasTime
                                    }

                                    PlasmaComponents3.Label {
                                        id: endTimeLabel

                                        readonly property bool endsToday: modelData.endDateTime - monthView.currentDate <= 86400000 // 24hrs in ms
                                        readonly property bool endsTomorrowInLessThan12Hours: modelData.endDateTime - monthView.currentDate <= 86400000 + 43200000 // 36hrs in ms

                                        Layout.row: 1
                                        Layout.column: 1
                                        Layout.minimumWidth: dateLabelMetrics.width

                                        text: endsToday || endsTomorrowInLessThan12Hours
                                                ? Qt.formatTime(modelData.endDateTime)
                                                : agenda.formatDateWithoutYear(modelData.endDateTime)
                                        horizontalAlignment: Qt.AlignRight
                                        opacity: 0.7

                                        visible: eventItem.hasTime
                                    }

                                    PlasmaComponents3.Label {
                                        id: eventTitle

                                        readonly property bool wrap: eventDescription.text === ""

                                        Layout.row: 0
                                        Layout.rowSpan: wrap ? 2 : 1
                                        Layout.column: 2
                                        Layout.fillWidth: true

                                        elide: Text.ElideRight
                                        text: modelData.title
                                        verticalAlignment: Text.AlignVCenter
                                        maximumLineCount: 2
                                        wrapMode: wrap ? Text.Wrap : Text.NoWrap
                                    }

                                    PlasmaComponents3.Label {
                                        id: eventDescription

                                        opacity: 0.7

                                        Layout.row: 1
                                        Layout.column: 2
                                        Layout.fillWidth: true

                                        elide: Text.ElideRight
                                        text: modelData.description
                                        verticalAlignment: Text.AlignVCenter
                                        enabled: false

                                        visible: text !== ""
                                    }
                                }
                            }
                        }
                    }
                }

                PlasmaExtras.Heading {
                    anchors.fill: holidaysView
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    anchors.leftMargin: units.largeSpacing
                    anchors.rightMargin: units.largeSpacing
                    text: monthView.isToday(monthView.currentDate) ? i18n("No events for today")
                                                                : i18n("No events for this day");
                    level: 3
                    enabled: false
                    visible: holidaysList.count == 0
                }
            }

            // Horizontal separator line between events and time zones
            PlasmaCore.SvgItem {
                visible: worldClocks.visible && agenda.visible

                Layout.fillWidth: true
                Layout.preferredHeight: naturalSize.height

                elementId: "horizontal-line"
                svg: PlasmaCore.Svg {
                    imagePath: "widgets/line"
                }
            }

            // Clocks stuff
            // ------------
            // Header text
            PlasmaExtras.Heading {
                visible: worldClocks.visible

                Layout.fillWidth: true

                level: 2

                text: i18n("Time Zones")
                maximumLineCount: 1
                elide: Text.ElideRight
            }

            // Clocks view itself
            PlasmaExtras.ScrollArea {
                id: worldClocks
                visible: calendar.showClocks

                Layout.fillWidth: true
                Layout.fillHeight: !agenda.visible
                Layout.leftMargin: -units.smallSpacing

                ListView {
                    id: clocksList

                    width: parent.width

                    model: {
                        var timezones = [];
                        for (var i = 0; i < plasmoid.configuration.selectedTimeZones.length; i++) {
                            timezones.push(plasmoid.configuration.selectedTimeZones[i]);
                        }

                        return timezones;
                    }

                    delegate: PlasmaExtras.ListItem {
                        id: listItem
                        readonly property bool isCurrentTimeZone: modelData === plasmoid.configuration.lastSelectedTimezone
                        separatorVisible: false

                        width: clocksList.width
                        height: units.gridUnit + units.smallSpacing

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: plasmoid.configuration.lastSelectedTimezone = modelData
                        }

                        RowLayout {
                            anchors.fill: parent

                            PlasmaComponents3.Label {
                                text: root.nameForZone(modelData)
                                font.weight: listItem.isCurrentTimeZone ? Font.Bold : Font.Normal
                                maximumLineCount: 1
                                elide: Text.ElideRight
                            }

                            PlasmaComponents3.Label {
                                Layout.fillWidth: true
                                horizontalAlignment: Qt.AlignRight
                                text: root.timeForZone(modelData)
                                font.weight: listItem.isCurrentTimeZone ? Font.Bold : Font.Normal
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }
                        }
                    }
                }
            }
        }
    }
}
