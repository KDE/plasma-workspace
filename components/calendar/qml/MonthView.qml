/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.workspace.calendar as PlasmaCalendar

Item {
    id: root

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
    property PlasmaCalendar.EventPluginsManager eventPluginsManager

    property date currentDate
    property string todayAuxilliaryText: ""
    // Bind to todayAuxilliaryText so sublabel can be visible on debut
    property string currentDateAuxilliaryText: todayAuxilliaryText

    property date showDate: new Date()

    property int borderWidth: 1
    property real borderOpacity: 0.4

    property int columns: calendarBackend.days
    property int rows: calendarBackend.weeks

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
    property alias currentIndex: swipeView.currentIndex

    property alias cellHeight: mainDaysCalendar.cellHeight
    readonly property alias daysModel: calendarBackend.daysModel

    KeyNavigation.up: viewHeader.previousButton
    // The view can have no highlighted item, so always highlight the first item
    Keys.onDownPressed: event => {
        swipeView.currentItem.focusFirstCellOfView();
    }

    function isToday(date: date): bool {
        return date.toDateString() === new Date().toDateString();
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
        swipeView.currentItem.finishChangeIfNeeded();
        swipeView.currentItem.resetViewPosition();
        swipeView.currentItem.incrementCurrentIndex();
    }

    /**
     * Go to the previous month/year/decade depending on the current
     * calendar view displayed.
     */
    function previousView() {
        swipeView.currentItem.finishChangeIfNeeded();
        swipeView.currentItem.resetViewPosition();
        swipeView.currentItem.decrementCurrentIndex();
    }

    /**
     * \return CalendarView
     */
    readonly property /*MonthView.CalendarView*/int calendarViewDisplayed: {
        switch (swipeView.currentIndex) {
        case 0:
            return MonthView.CalendarView.DayView;
        case 1:
            return MonthView.CalendarView.MonthView;
        case 2:
        default:
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

    Connections {
        target: eventPluginsManager
        function onPluginsChanged() {
            root.todayAuxilliaryText = "";
        }
    }

    PlasmaCalendar.Calendar {
        id: calendarBackend

        days: 7
        weeks: 6
        firstDayOfWeek: Qt.locale().firstDayOfWeek
        today: root.today

        Component.onCompleted: {
            daysModel.setPluginsManager(root.eventPluginsManager);
        }

        onYearChanged: {
            root.updateYearOverview()
            root.updateDecadeOverview()
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
                    isCurrent: true,
                })
            }
            root.updateYearOverview()
        }
    }

    ListModel {
        id: yearModel

        Component.onCompleted: {
            for (let i = 0; i < 12; ++i) {
                append({
                    label: 2050, // this value will be overwritten, but it set the type of the property to int
                    yearNumber: 2050,
                    isCurrent: (i > 0 && i < 11), // first and last year are outside the decade
                })
            }
            root.updateDecadeOverview()
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

    PlasmaComponents.SwipeView {
        id: swipeView

        // Warning: this binding depends on an implementation detail of PlasmaComponents.SwipeView
        readonly property bool moving: (contentItem as ListView)?.moving ?? false

        anchors {
            top: viewHeader.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        activeFocusOnTab: false
        clip: true

        // cross-block interactiveness to avoid swiping in the orthogonal
        // directions simultaneously and also to mitigate QTBUG-126042
        interactive: !(mainDaysCalendar.moving || yearView.moving || decadeView.moving)

        KeyNavigation.left: root.showDigitalClockHeader ? root.KeyNavigation.left : viewHeader.tabBar
        KeyNavigation.tab: viewHeader.tabButton
        Keys.onLeftPressed: event => {
            Keys.upPressed(event);
        }
        Keys.onUpPressed: event => {
            viewHeader.tabBar.currentItem.forceActiveFocus(Qt.BacktabFocusReason);
        }

        onCurrentIndexChanged: if (currentIndex > 1) {
            root.updateDecadeOverview();
        }

        // Reduce visual glitches when resizing the view.
        onWidthChanged: resetCurrentIndexSkippingAnimation()
        onHeightChanged: resetCurrentIndexSkippingAnimation()

        function resetCurrentIndexSkippingAnimation() {
            // Resetting currentIndex to -1 for a moment causes SwipeView to
            // forget about its scheduled animation.
            const index = currentIndex;
            currentIndex = -1;
            currentIndex = index;
        }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse
            orientation: Qt.Vertical
            parent: swipeView.currentItem
            onWheel: wheel => {
                // magic number 15 for common "one scroll"
                // See https://doc.qt.io/qt-6/qml-qtquick-wheelhandler.html#rotation-prop
                while (rotation >= 15) {
                    rotation -= 15;
                    root.previousView();
                }
                while (rotation <= -15) {
                    rotation += 15;
                    root.nextView();
                }
            }
        }

        // MonthView
        InfiniteList {
            id: mainDaysCalendar

            function handleUpPress(event: var) {
                if (root.showDigitalClockHeader) {
                    root.Keys.upPressed(event);
                    return;
                }
                swipeView.Keys.upPressed(event);
            }

            // cross-block interactiveness to avoid swiping in the orthogonal
            // directions simultaneously and also to mitigate QTBUG-126042
            interactive: !swipeView.moving

            backend: calendarBackend
            viewType: InfiniteList.ViewType.DayView

            delegate: DaysCalendar {
                required property int index

                dateMatchingPrecision: PlasmaCalendar.Calendar.MatchYearMonthAndDay
                columns: calendarBackend.days
                rows: calendarBackend.weeks
                width: mainDaysCalendar.width
                height: mainDaysCalendar.height
                borderWidth: root.borderWidth
                showWeekNumbers: root.showWeekNumbers
                dayOfWeekHeaderModel: calendarBackend.days

                backend: {
                    switch (index) {
                    case 0:
                        return mainDaysCalendar.previousCalendar;
                    case 1:
                        return calendarBackend;
                    case 2:
                        return mainDaysCalendar.nextCalendar;
                    }
                }
                gridModel: {
                    switch (index) {
                    case 0:
                        return mainDaysCalendar.previousModel;
                    case 1:
                        return calendarBackend.daysModel;
                    case 2:
                        return mainDaysCalendar.nextModel;
                    }
                }

                KeyNavigation.left: swipeView.KeyNavigation.left
                KeyNavigation.tab: swipeView.KeyNavigation.tab
                Keys.onUpPressed: event => {
                    mainDaysCalendar.handleUpPress(event);
                }

                onActivated: (index, date, item) => {
                    const rowNumber = Math.floor(index / root.columns);
                    root.currentDate = new Date(date.yearNumber, date.monthNumber - 1, date.dayNumber)

                    if (date.subLabel) {
                        root.currentDateAuxilliaryText = date.subLabel;
                    }
                }
            }
        }

        // YearView
        InfiniteList {
            id: yearView

            // cross-block interactiveness to avoid swiping in the orthogonal
            // directions simultaneously and also to mitigate QTBUG-126042
            interactive: !swipeView.moving

            backend: calendarBackend
            viewType: InfiniteList.ViewType.YearView
            delegate: DaysCalendar {
                dateMatchingPrecision: PlasmaCalendar.Calendar.MatchYearAndMonth
                columns: 3
                rows: 4
                width: yearView.width
                height: yearView.height
                borderWidth: root.borderWidth
                gridModel: monthModel

                KeyNavigation.left: swipeView.KeyNavigation.left
                KeyNavigation.tab: swipeView.KeyNavigation.tab
                Keys.onUpPressed: event => {
                    mainDaysCalendar.handleUpPress(event);
                }

                onActivated: (index, date, item) => {
                    calendarBackend.goToMonth(date.monthNumber);
                    swipeView.currentIndex = 0;
                }
            }
        }

        // DecadeView
        InfiniteList {
            id: decadeView

            // cross-block interactiveness to avoid swiping in the orthogonal
            // directions simultaneously and also to mitigate QTBUG-126042
            interactive: !swipeView.moving

            backend: calendarBackend
            viewType: InfiniteList.ViewType.DecadeView
            delegate: DaysCalendar {
                required property int index

                readonly property int decade: {
                    const year = calendarBackend.displayedDate.getFullYear();
                    return year - year % 10;
                }

                dateMatchingPrecision: PlasmaCalendar.Calendar.MatchYear
                columns: 3
                rows: 4
                width: decadeView.width
                height: decadeView.height
                borderWidth: root.borderWidth

                gridModel: {
                    switch (index) {
                    case 0:
                        return decadeView.previousModel;
                    case 1:
                        return yearModel;
                    case 2:
                        return decadeView.nextModel;
                    }
                }

                KeyNavigation.left: swipeView.KeyNavigation.left
                KeyNavigation.tab: swipeView.KeyNavigation.tab
                Keys.onUpPressed: event => {
                    mainDaysCalendar.handleUpPress(event);
                }

                onActivated: (index, date, item) => {
                    calendarBackend.goToYear(date.yearNumber);
                    swipeView.currentIndex = 1;
                }
            }
        }
    }

    Component.onCompleted: {
        root.currentDate = calendarBackend.today;
    }
}
