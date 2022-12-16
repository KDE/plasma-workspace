/*
    SPDX-FileCopyrightText: 2013 Mark Gaiser <markg85@gmail.com>
    SPDX-FileCopyrightText: 2016 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDate>
#include <QJsonArray>
#include <QObject>

#include "daydata.h"
#include "daysmodel.h"

class QAbstractItemModel;
class CalendarPrivate;

class Calendar : public QObject
{
    Q_OBJECT
    /* The conversion between QDate and JS Date is broken. The specification says that a date
     * is represented by the start of the UTC day, but for negative to UTC timezones this results
     * in wrong dates: Jan 2 in C++ -> Jan 2 (00:00) UTC -> Jan 1 (23:00) UTC-1 in JS.
     * So use QDateTime for interfacing to always carry a timezone around.
     * https://bugreports.qt.io/browse/QTBUG-29328 */

    /**
     * This property is used to determine which data from which month to show, it ensures
     * the day passed in the QDate is visible
     */
    Q_PROPERTY(QDateTime displayedDate READ displayedDate WRITE setDisplayedDate NOTIFY displayedDateChanged)

    /**
     * This property is used to determine which data from which month to show, it ensures
     * the day passed in the QDate is visible
     */
    Q_PROPERTY(QDateTime today READ today WRITE setToday NOTIFY todayChanged)

    /**
     * This determines which kind of data types should be contained in
     * selectedDayModel and upcomingEventsModel. By default all types are included.
     * NOTE: Only the Event type is fully implemented.
     * TODO: Fully implement the other data types throughout this code.
     */
    Q_PROPERTY(int types READ types WRITE setTypes NOTIFY typesChanged)

    /**
     * This model contains the week numbers for the given date grid.
     */
    Q_PROPERTY(QJsonArray weeksModel READ weeksModel NOTIFY weeksModelChanged)

    /**
     * The number of days a week contains.
     * TODO: perhaps this one can just be removed. A week is 7 days by definition.
     * However, i don't know if that's also the case in other more exotic calendars.
     */
    Q_PROPERTY(int days READ days WRITE setDays NOTIFY daysChanged)

    /**
     * The number of weeks that the model property should contain.
     */
    Q_PROPERTY(int weeks READ weeks WRITE setWeeks NOTIFY weeksChanged)

    /**
     * The start day of a week. By default this follows current Locale. It can be
     * changed. One then needs to use the numbers in the Qt DayOfWeek enum:
     *
     *    Monday = 1
     *    Tuesday = 2
     *    Wednesday = 3
     *    Thursday = 4
     *    Friday = 5
     *    Saturday = 6
     *    Sunday = 7
     *
     * This value doesn't do anything to other data structures, but helps you
     * visualizing the data.
     *
     * WARNING: QML has different enum values for week days - Sunday is 0, this function
     *          automatically converts that on READ and WRITE and it's stored as QDate format
     *          (ie. the one above). So firstDayOfWeek() call from QML would return 0 for Sunday
     *          while internally it's 7 and vice-versa.
     */
    Q_PROPERTY(int firstDayOfWeek READ firstDayOfWeek WRITE setFirstDayOfWeek NOTIFY firstDayOfWeekChanged)

    /**
     * The full year in a numeric value. For example 2013, not 13.
     */
    Q_PROPERTY(int year READ year NOTIFY yearChanged)

    /**
     * If an error occurred, it will be set in this string as human readable text.
     */
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

    /**
     * This is the human readable long month name. So not "Feb" but "February".
     * TODO: this should either be done in QML using javascript or by making a
     *       function available because this is limiting. There are places
     *       where you would want the short month name.
     */
    Q_PROPERTY(QString monthName READ monthName NOTIFY monthNameChanged)
    Q_PROPERTY(int month READ month NOTIFY monthChanged)

    /**
     * This model contains the actual grid data of days. For example, if you had set:
     * - days = 7 (7 days in one week)
     * - weeks = 6 (6 weeks in one month view)
     * then this model will contain 42 entries (days * weeks). Each entry contains
     * metadata about the current day. The exact metadata can be found in "daysmodel.cpp"
     * where the exact names usable in QML are being set.
     */
    Q_PROPERTY(QAbstractItemModel *daysModel READ daysModel CONSTANT)

public:
    enum Type {
        Holiday = 1,
        Event = 2,
        Todo = 4,
        Journal = 8,
    };
    Q_ENUM(Type)
    Q_DECLARE_FLAGS(Types, Type)

    enum DateMatchingPrecision {
        MatchYear,
        MatchYearAndMonth,
        MatchYearMonthAndDay,
    };
    Q_ENUM(DateMatchingPrecision)

    explicit Calendar(QObject *parent = nullptr);
    ~Calendar() override;

    // Displayed date
    QDateTime displayedDate() const;
    void setDisplayedDate(const QDate &dateTime);
    void setDisplayedDate(const QDateTime &dateTime);

    // The day that represents "today"
    QDateTime today() const;
    void setToday(const QDateTime &dateTime);

    // Types
    int types() const;
    void setTypes(int types);

    // Days
    int days();
    void setDays(int days);

    // Weeks
    int weeks() const;
    void setWeeks(int weeks);

    // Start day
    int firstDayOfWeek() const;
    void setFirstDayOfWeek(int day);

    // Error message
    QString errorMessage() const;

    // Month name
    QString monthName() const;
    int month() const;
    int year() const;

    // Models
    QAbstractItemModel *daysModel() const;
    QJsonArray weeksModel() const;

    // QML invokables
    Q_INVOKABLE void nextMonth();
    Q_INVOKABLE void previousMonth();
    Q_INVOKABLE void nextYear();
    Q_INVOKABLE void previousYear();
    Q_INVOKABLE void nextDecade();
    Q_INVOKABLE void previousDecade();
    Q_INVOKABLE QString dayName(int weekday) const;
    Q_INVOKABLE int currentWeek() const;
    Q_INVOKABLE void resetToToday();
    Q_INVOKABLE void goToMonth(int month);
    Q_INVOKABLE void goToYear(int year);

Q_SIGNALS:
    void displayedDateChanged();
    void todayChanged();
    void typesChanged();
    void daysChanged();
    void weeksChanged();
    void firstDayOfWeekChanged();
    void errorMessageChanged();
    void monthNameChanged();
    void monthChanged();
    void yearChanged();
    void weeksModelChanged();

public Q_SLOTS:
    void updateData();

private:
    CalendarPrivate *const d;
};
