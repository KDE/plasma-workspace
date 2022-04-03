/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.4
import QtQuick.Layouts 1.1
import QtQml 2.15

import org.kde.kquickcontrolsaddons 2.0 // For kcmshell
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.calendar 2.0 as PlasmaCalendar
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.private.digitalclock 1.0

// Top-level layout containing:
// - Left column with world clock and agenda view
// - Right column with current date header and calendar
PlasmaExtras.Representation {
    id: calendar

    // The "sensible" values
    property int _minimumWidth: (calendar.showAgenda || calendar.showClocks) ? PlasmaCore.Units.gridUnit * 45 : PlasmaCore.Units.gridUnit * 22
    property int _minimumHeight: PlasmaCore.Units.gridUnit * 25

    PlasmaCore.ColorScope.inherit: false
    PlasmaCore.ColorScope.colorGroup: PlasmaCore.Theme.NormalColorGroup

    Layout.minimumWidth: _minimumWidth
    Layout.minimumHeight: _minimumHeight
    Layout.preferredWidth: _minimumWidth
    Layout.preferredHeight: _minimumHeight
    Layout.maximumWidth: _minimumWidth
    Layout.maximumHeight: _minimumHeight

    collapseMarginsHint: true

    readonly property int paddings: PlasmaCore.Units.smallSpacing
    readonly property bool showAgenda: PlasmaCalendar.EventPluginsManager.enabledPlugins.length > 0
    readonly property bool showClocks: Plasmoid.configuration.selectedTimeZones.length > 1

    property alias borderWidth: monthView.borderWidth
    property alias monthView: monthView

    property bool debug: false

    property bool isExpanded: Plasmoid.expanded

    onIsExpandedChanged: {
        // clear all the selections when the plasmoid is showing/hiding
        monthView.resetToToday();
    }

    // Header containing date and pin button
    header: PlasmaExtras.PlasmoidHeading {
        id: headerArea
        implicitHeight: calendarHeader.implicitHeight

        // Agenda view header
        // -----------------
        ColumnLayout {
            id: eventHeader

            anchors.left: parent.left
            width: visible ? parent.width / 2 - 1 : 0

            visible: calendar.showAgenda || calendar.showClocks
            RowLayout {
                PlasmaExtras.Heading {
                    Layout.fillWidth: true
                    Layout.leftMargin: calendar.paddings // Match calendar title

                    text: monthView.currentDate.toLocaleDateString(Qt.locale(), Locale.LongFormat)
                }
            }
            RowLayout {
                // Heading text
                PlasmaExtras.Heading {
                    visible: agenda.visible

                    Layout.fillWidth: true
                    Layout.leftMargin: calendar.paddings

                    level: 2

                    text: i18n("Events")
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }
                PlasmaComponents3.ToolButton {
                    visible: agenda.visible && ApplicationIntegration.calendarInstalled
                    text: i18nc("@action:button Add event", "Add…")
                    Layout.rightMargin: calendar.paddings
                    icon.name: "list-add"
                    onClicked: ApplicationIntegration.launchCalendar()
                }
            }
        }

        // Vertical separator line between columns
        // =======================================
        PlasmaCore.SvgItem {
            id: headerSeparator
            anchors.left: eventHeader.right
            anchors.bottomMargin: PlasmaCore.Units.smallSpacing * 2
            width: visible ? 1 : 0
            height: calendarHeader.height - PlasmaCore.Units.smallSpacing * 2
            visible: eventHeader.visible

            elementId: "vertical-line"
            svg: PlasmaCore.Svg {
                imagePath: "widgets/line"
            }
        }

        GridLayout {
            id: calendarHeader
            width: calendar.showAgenda || calendar.showClocks ? parent.width / 2 : parent.width
            anchors.left: headerSeparator.right
            columns: 6
            rows: 2

            PlasmaExtras.Heading {
                Layout.row: 0
                Layout.column: 0
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.leftMargin: calendar.paddings + PlasmaCore.Units.smallSpacing
                text: monthView.selectedYear === (new Date()).getFullYear() ? monthView.selectedMonth : i18nc("Format: month year", "%1 %2", monthView.selectedMonth, monthView.selectedYear.toString())
            }

            PlasmaComponents3.ToolButton {
                Layout.row: 0
                Layout.column: 4
                Layout.alignment: Qt.AlignRight
                visible: Plasmoid.action("configure").enabled
                icon.name: "configure"
                onClicked: Plasmoid.action("configure").trigger()
                PlasmaComponents3.ToolTip {
                    text: Plasmoid.action("configure").text
                }
            }

            // Allows the user to keep the calendar open for reference
            PlasmaComponents3.ToolButton {
                Layout.row: 0
                Layout.column: 5
                checkable: true
                checked: Plasmoid.configuration.pin
                onToggled: Plasmoid.configuration.pin = checked
                icon.name: "window-pin"
                PlasmaComponents3.ToolTip {
                    text: i18n("Keep Open")
                }
            }

            PlasmaComponents3.TabBar {
                id: tabbar
                currentIndex: monthView.currentIndex
                Layout.row: 1
                Layout.column: 0
                Layout.columnSpan: 3
                Layout.topMargin: PlasmaCore.Units.smallSpacing
                Layout.fillWidth: true
                Layout.leftMargin: PlasmaCore.Units.smallSpacing

                PlasmaComponents3.TabButton {
                    text: i18n("Days");
                    onClicked: monthView.showMonthView();
                    display: PlasmaComponents3.AbstractButton.TextOnly
                }
                PlasmaComponents3.TabButton {
                    text: i18n("Months");
                    onClicked: monthView.showYearView();
                    display: PlasmaComponents3.AbstractButton.TextOnly
                }
                PlasmaComponents3.TabButton {
                    text: i18n("Years");
                    onClicked: monthView.showDecadeView();
                    display: PlasmaComponents3.AbstractButton.TextOnly
                }
            }

            PlasmaComponents3.ToolButton {
                id: previousButton
                property string tooltip
                Layout.row: 1
                Layout.column: 3

                Layout.leftMargin: PlasmaCore.Units.smallSpacing
                Layout.bottomMargin: PlasmaCore.Units.smallSpacing
                icon.name: Qt.application.layoutDirection === Qt.RightToLeft ? "go-next" : "go-previous"
                onClicked: monthView.previousView()
                Accessible.name: tooltip
                PlasmaComponents3.ToolTip {
                    text: {
                        switch(monthView.calendarViewDisplayed) {
                            case PlasmaCalendar.MonthView.CalendarView.DayView:
                                return i18n("Previous month")
                            case PlasmaCalendar.MonthView.CalendarView.MonthView:
                                return i18n("Previous year")
                            case PlasmaCalendar.MonthView.CalendarView.YearView:
                                return i18n("Previous decade")
                            default:
                                return "";
                        }
                    }
                }
            }

            PlasmaComponents3.ToolButton {
                Layout.bottomMargin: PlasmaCore.Units.smallSpacing
                Layout.row: 1
                Layout.column: 4
                onClicked: monthView.resetToToday()
                text: i18ndc("libplasma5", "Reset calendar to today", "Today")
                Accessible.description: i18nd("libplasma5", "Reset calendar to today")
            }

            PlasmaComponents3.ToolButton {
                id: nextButton
                property string tooltip
                Layout.bottomMargin: PlasmaCore.Units.smallSpacing
                Layout.row: 1
                Layout.column: 5

                icon.name: Qt.application.layoutDirection === Qt.RightToLeft ? "go-previous" : "go-next"
                onClicked: monthView.nextView()
                Accessible.name: tooltip
                PlasmaComponents3.ToolTip {
                    text: {
                        switch(monthView.calendarViewDisplayed) {
                            case PlasmaCalendar.MonthView.CalendarView.DayView:
                                return i18n("Next month")
                            case PlasmaCalendar.MonthView.CalendarView.MonthView:
                                return i18n("Next year")
                            case PlasmaCalendar.MonthView.CalendarView.YearView:
                                return i18n("Next decade")
                            default:
                                return "";
                        }
                    }
                }
            }
        }
    }

    // Left column containing agenda view and time zones
    // ==================================================
    ColumnLayout {
        id: leftColumn

        visible: calendar.showAgenda || calendar.showClocks
        width: parent.width / 2 - 1
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }


        // Agenda view itself
        Item {
            id: agenda
            visible: calendar.showAgenda

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: PlasmaCore.Units.gridUnit * 4

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
                const values1 = [
                    date1.getFullYear(),
                    date1.getMonth(),
                    date1.getDate()
                ];

                const values2 = [
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

                function onCurrentDateChanged() {
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

                function onAgendaUpdated(updatedDate) {
                    if (agenda.dateEquals(updatedDate, monthView.currentDate)) {
                        holidaysList.model = null;
                        holidaysList.model = monthView.daysModel.eventsForDate(monthView.currentDate);
                    }
                }
            }

            Connections {
                target: Plasmoid.configuration

                onEnabledCalendarPluginsChanged: {
                    PlasmaCalendar.EventPluginsManager.enabledPlugins = Plasmoid.configuration.enabledCalendarPlugins;
                }
            }

            Binding {
                target: Plasmoid.self
                property: "hideOnWindowDeactivate"
                value: !Plasmoid.configuration.pin
                restoreMode: Binding.RestoreBinding
            }

            TextMetrics {
                id: dateLabelMetrics

                // Date/time are arbitrary values with all parts being two-digit
                readonly property string timeString: Qt.formatTime(new Date(2000, 12, 12, 12, 12, 12, 12))
                readonly property string dateString: agenda.formatDateWithoutYear(new Date(2000, 12, 12, 12, 12, 12))

                font: PlasmaCore.Theme.defaultFont
                text: timeString.length > dateString.length ? timeString : dateString
            }

            PlasmaComponents3.ScrollView {
                id: holidaysView
                anchors.fill: parent

                // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
                PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

                ListView {
                    id: holidaysList
                    highlight: Item {}

                    delegate: PlasmaComponents3.ItemDelegate {
                        id: eventItem
                        width: holidaysList.width
                        padding: calendar.paddings
                        leftPadding: calendar.paddings + PlasmaCore.Units.smallSpacing * 2
                        text: eventTitle.text
                        hoverEnabled: true
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
                            const startIsMidnight = modelData.startDateTime.getHours() === 0
                                            && modelData.startDateTime.getMinutes() === 0;

                            const endIsMidnight = modelData.endDateTime.getHours() === 0
                                            && modelData.endDateTime.getMinutes() === 0;

                            const sameDay = modelData.startDateTime.getDate() === modelData.endDateTime.getDate()
                                    && modelData.startDateTime.getDay() === modelData.endDateTime.getDay()

                            return !(startIsMidnight && endIsMidnight && sameDay);
                        }

                        PlasmaComponents3.ToolTip {
                            text: modelData.description
                            visible: text !== "" && eventItem.hovered
                        }

                        contentItem: GridLayout {
                            id: eventGrid
                            columns: 3
                            rows: 2
                            rowSpacing: 0
                            columnSpacing: 2 * PlasmaCore.Units.smallSpacing

                            Rectangle {
                                id: eventColor

                                Layout.row: 0
                                Layout.column: 0
                                Layout.rowSpan: 2
                                Layout.fillHeight: true

                                color: modelData.eventColor
                                width: 5 * PlasmaCore.Units.devicePixelRatio
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

                                Layout.row: 0
                                Layout.column: 2
                                Layout.fillWidth: true

                                elide: Text.ElideRight
                                text: modelData.title
                                verticalAlignment: Text.AlignVCenter
                                maximumLineCount: 2
                            }
                        }
                    }
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: holidaysView
                width: holidaysView.width - (PlasmaCore.Units.largeSpacing * 8)

                visible: holidaysList.count == 0

                iconName: "checkmark"
                text: monthView.isToday(monthView.currentDate) ? i18n("No events for today")
                                                               : i18n("No events for this day");
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
        // Header text + button to change time & timezone
        PlasmaExtras.PlasmoidHeading {
            visible: worldClocks.visible
            leftInset: 0
            rightInset: 0
            rightPadding: PlasmaCore.Units.smallSpacing
            contentItem: RowLayout {
                PlasmaExtras.Heading {
                    Layout.leftMargin: calendar.paddings + PlasmaCore.Units.smallSpacing * 2
                    Layout.fillWidth: true

                    level: 2

                    text: i18n("Time Zones")
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }

                PlasmaComponents3.ToolButton {
                    visible: KCMShell.authorize("kcm_clock.desktop").length > 0
                    text: i18n("Switch…")
                    icon.name: "preferences-system-time"
                    onClicked: KCMShell.openSystemSettings("kcm_clock")

                    PlasmaComponents3.ToolTip {
                        text: i18n("Switch to another timezone")
                    }
                }
            }
        }

        // Clocks view itself
        PlasmaComponents3.ScrollView {
            id: worldClocks
            visible: calendar.showClocks

            Layout.fillWidth: true
            Layout.fillHeight: !agenda.visible
            Layout.minimumHeight: visible ? PlasmaCore.Units.gridUnit * 7 : 0
            Layout.maximumHeight: agenda.visible ? PlasmaCore.Units.gridUnit * 10 : -1

            // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
            PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

            ListView {
                id: clocksList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: PlasmaCore.Units.smallSpacing * 2

                highlight: Item {}

                model: {
                    let timezones = [];
                    for (let i = 0; i < Plasmoid.configuration.selectedTimeZones.length; i++) {
                        timezones.push(Plasmoid.configuration.selectedTimeZones[i]);
                    }

                    return timezones;
                }

                delegate: PlasmaComponents3.ItemDelegate {
                    id: listItem
                    readonly property bool isCurrentTimeZone: modelData === Plasmoid.configuration.lastSelectedTimezone
                    width: clocksList.width
                    padding: calendar.paddings
                    leftPadding: calendar.paddings + PlasmaCore.Units.smallSpacing * 2

                    contentItem: RowLayout {
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

    // Vertical separator line between columns
    // =======================================
    PlasmaCore.SvgItem {
        id: mainSeparator
        visible: leftColumn.visible
        anchors {
            right: monthViewWrapper.left
            top: parent.top
            bottom: parent.bottom
        }
        width: 1

        elementId: "vertical-line"
        svg: PlasmaCore.Svg {
            imagePath: "widgets/line"
        }
    }

    // Right column containing calendar
    // ===============================
    FocusScope {
        id: monthViewWrapper
        width: calendar.showAgenda || calendar.showClocks ? parent.width / 2 : parent.width
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        PlasmaCalendar.MonthView {
            id: monthView
            anchors.margins: PlasmaCore.Units.smallSpacing
            borderOpacity: 0.25
            today: root.tzDate
            firstDayOfWeek: Plasmoid.configuration.firstDayOfWeek > -1
                ? Plasmoid.configuration.firstDayOfWeek
                : Qt.locale().firstDayOfWeek
            showWeekNumbers: Plasmoid.configuration.showWeekNumbers
            showCustomHeader: true
        }
    }
}
