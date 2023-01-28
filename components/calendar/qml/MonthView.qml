/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.15
import QtQuick.Layouts 1.1

import org.kde.plasma.workspace.calendar 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: root

    anchors.fill: parent // TODO KF6 don't use anchors

    /**
     * Currently selected month name.
     * \property string MonthView::selectedMonth
     */
    property alias selectedMonth: calendarBackend.monthName
    /**
     * Currently selected month year.
     * \property int MonthView::selectedYear
     */
    property alias selectedYear: calendarBackend.year
    /**
     * The start day of a week.
     * \property int MonthView::firstDayOfWeek
     * \sa Calendar::firstDayOfWeek
     */
    property alias firstDayOfWeek: calendarBackend.firstDayOfWeek
    /**
     * The event plugins manager used in the view
     */
    property QtObject eventPluginsManager

    property QtObject date
    property date currentDate
    property string todayAuxilliaryText: ""
    // Bind to todayAuxilliaryText so sublabel can be visible on debut
    property string currentDateAuxilliaryText: todayAuxilliaryText

    property date showDate: new Date()

    property int borderWidth: 1
    property real borderOpacity: 0.4

    property int columns: calendarBackend.days
    property int rows: calendarBackend.weeks

    property Item selectedItem
    property int week;
    property int firstDay: new Date(showDate.getFullYear(), showDate.getMonth(), 1).getDay()
    property alias today: calendarBackend.today
    property bool showWeekNumbers: false

    // properties for communicating with the digital clock applet
    property bool showDigitalClockHeader: false
    property var digitalClock: null
    property var eventButton: null
    property alias viewHeader: viewHeader

    /**
     * SwipeView currentIndex needed for binding a TabBar to the MonthView.
     */
    property int currentIndex: swipeView.currentIndex

    property alias cellHeight: mainDaysCalendar.cellHeight
    property QtObject daysModel: calendarBackend.daysModel

    KeyNavigation.up: viewHeader.previousButton
    // The view can have no highlighted item, so always highlight the first item
    Keys.onDownPressed: swipeView.currentItem.focusFirstCellOfView()
    signal upPressed(var event)

    function isToday(date) {
        return date.toDateString() === new Date().toDateString();
    }

    function eventDate(yearNumber,monthNumber,dayNumber) {
        const d = new Date(yearNumber, monthNumber-1, dayNumber);
        return Qt.formatDate(d, "dddd dd MMM yyyy");
    }

    function updateYearOverview() {
        const date = calendarBackend.displayedDate;
        const day = date.getDate();
        const year = date.getFullYear();

        for (let i = 0, j = monthModel.count; i < j; ++i) {
            monthModel.setProperty(i, "yearNumber", year);
        }
    }

    function updateDecadeOverview() {
        const date = calendarBackend.displayedDate;
        const day = date.getDate();
        const month = date.getMonth() + 1;
        const year = date.getFullYear();
        const decade = year - year % 10;

        for (let i = 0, j = yearModel.count; i < j; ++i) {
            const label = decade - 1 + i;
            yearModel.setProperty(i, "yearNumber", label);
            yearModel.setProperty(i, "label", label);
        }
    }

    /**
     * Possible calendar views
     */
    enum CalendarView {
        DayView,
        MonthView,
        YearView
    }

    /**
     * Move calendar to month view showing today's date.
     */
    function resetToToday() {
        calendarBackend.resetToToday();
        root.currentDate = root.today;
        root.currentDateAuxilliaryText = root.todayAuxilliaryText;
        swipeView.currentIndex = 0;
    }

    /**
     * Go to the next month/year/decade depending on the current
     * calendar view displayed.
     */
    function nextView() {
        swipeView.currentItem.resetViewPosition();
        swipeView.currentItem.incrementCurrentIndex();
    }

    /**
     * Go to the previous month/year/decade depending on the current
     * calendar view displayed.
     */
    function previousView() {
        swipeView.currentItem.resetViewPosition();
        swipeView.currentItem.decrementCurrentIndex();
    }

    /**
     * \return CalendarView
     */
    readonly property var calendarViewDisplayed: {
        if (swipeView.currentIndex === 0) {
            return MonthView.CalendarView.DayView;
        } else if (swipeView.currentIndex === 1) {
            return MonthView.CalendarView.MonthView;
        } else if (swipeView.currentIndex === 2) {
            return MonthView.CalendarView.YearView;
        }
    }

    /**
     * Show month view.
     */
    function showMonthView() {
        swipeView.currentIndex = 0;
    }

    /**
     * Show year view.
     */
    function showYearView() {
        swipeView.currentIndex = 1;
    }

    /**
     * Show decade view.
     */
    function showDecadeView() {
        swipeView.currentIndex = 2;
    }

    Calendar {
        id: calendarBackend

        days: 7
        weeks: 6
        firstDayOfWeek: Qt.locale().firstDayOfWeek
        today: root.today

        Component.onCompleted: {
            daysModel.setPluginsManager(root.eventPluginsManager);
        }

        onYearChanged: {
            updateYearOverview()
            updateDecadeOverview()
        }
    }

    ListModel {
        id: monthModel

        Component.onCompleted: {
            for (let i = 0; i < 12; ++i) {
                append({
                    label: Qt.locale(Qt.locale().uiLanguages[0]).standaloneMonthName(i, Locale.LongFormat),
                    monthNumber: i + 1,
                    yearNumber: 2050,
                    isCurrent: true
                })
            }
            updateYearOverview()
        }
    }

    ListModel {
        id: yearModel

        Component.onCompleted: {
            for (let i = 0; i < 12; ++i) {
                append({
                    label: 2050, // this value will be overwritten, but it set the type of the property to int
                    yearNumber: 2050,
                    isCurrent: (i > 0 && i < 11) // first and last year are outside the decade
                })
            }
            updateDecadeOverview()
        }
    }

    /* ------------------------------------------------------- UI Starts From Here ----------------------------------------------------- */

    MonthViewHeader {
        id: viewHeader

        width: parent.width
        anchors.top: parent.top
        swipeView: swipeView
        monthViewRoot: root
    }

    PlasmaComponents3.SwipeView {
        id: swipeView
        anchors {
            top: viewHeader.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        activeFocusOnTab: false
        clip: true

        KeyNavigation.left: root.showDigitalClockHeader ? root.KeyNavigation.left : viewHeader.tabBar
        KeyNavigation.tab: viewHeader.tabButton
        Keys.onLeftPressed: Keys.onUpPressed(event);
        Keys.onUpPressed: viewHeader.tabBar.currentItem.forceActiveFocus(Qt.BacktabFocusReason);

        onCurrentIndexChanged: if (currentIndex > 1) {
            updateDecadeOverview();
        }

        // MonthView
        InfiniteList {
           id: mainDaysCalendar

           function handleUpPress(event) {
                if(root.showDigitalClockHeader) {
                    root.upPressed(event);
                    return;
                }
                swipeView.Keys.onUpPressed(event);
            }

           backend: calendarBackend
           viewType: InfiniteList.ViewType.DayView

           delegate: DaysCalendar {
                columns: calendarBackend.days
                rows: calendarBackend.weeks
                width: mainDaysCalendar.width
                height: mainDaysCalendar.height
                showWeekNumbers: root.showWeekNumbers

                headerModel: calendarBackend.days
                gridModel: calendarBackend.daysModel

                dateMatchingPrecision: Calendar.MatchYearMonthAndDay

                KeyNavigation.left: swipeView.KeyNavigation.left
                KeyNavigation.tab: swipeView.KeyNavigation.tab
                Keys.onUpPressed: mainDaysCalendar.handleUpPress(event)

                onActivated: {
                    const rowNumber = Math.floor(index / columns);
                    week = 1 + calendarBackend.weeksModel[rowNumber];
                    root.currentDate = new Date(date.yearNumber, date.monthNumber - 1, date.dayNumber)

                    if (date.subLabel) {
                        root.currentDateAuxilliaryText = date.subLabel;
                    }
                }
                onScrollUp: root.nextView()
                onScrollDown: root.previousView()
            }
        }

        // YearView
        InfiniteList {
           id: yearView

           backend: calendarBackend
           viewType: InfiniteList.ViewType.YearView
           delegate: DaysCalendar {
                columns: 3
                rows: 4

                dateMatchingPrecision: Calendar.MatchYearAndMonth
                width: yearView.width
                height: yearView.height
                gridModel: monthModel

                KeyNavigation.left: swipeView.KeyNavigation.left
                KeyNavigation.tab: swipeView.KeyNavigation.tab
                Keys.onUpPressed: mainDaysCalendar.handleUpPress(event)

                onActivated: {
                    calendarBackend.goToMonth(date.monthNumber);
                    swipeView.currentIndex = 0;
                }
                onScrollUp: root.nextView()
                onScrollDown: root.previousView()
            }
        }

        // DecadeView
        InfiniteList {
            id: decadeView

            backend: calendarBackend
            viewType: InfiniteList.ViewType.DecadeView
            delegate: DaysCalendar {
                readonly property int decade: {
                    const year = calendarBackend.displayedDate.getFullYear()
                    return year - year % 10
                }

                columns: 3
                rows: 4
                width: decadeView.width
                height: decadeView.height
                dateMatchingPrecision: Calendar.MatchYear

                gridModel: yearModel

                KeyNavigation.left: swipeView.KeyNavigation.left
                KeyNavigation.tab: swipeView.KeyNavigation.tab
                Keys.onUpPressed: mainDaysCalendar.handleUpPress(event)

                onActivated: {
                    calendarBackend.goToYear(date.yearNumber);
                    swipeView.currentIndex = 1;
                }

                onScrollUp: root.nextView()
                onScrollDown: root.previousView()
            }
        }
    }

    Component.onCompleted: {
        root.currentDate = calendarBackend.today
    }
}
