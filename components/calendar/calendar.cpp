/*
    SPDX-FileCopyrightText: 2013 Mark Gaiser <markg85@gmail.com>
    SPDX-FileCopyrightText: 2016 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDebug>

#include "calendar.h"

class CalendarPrivate
{
public:
    explicit CalendarPrivate(Calendar *q);

    QDate displayedDate;
    QDate today;
    Calendar::Types types;
    QList<DayData> dayList;
    DaysModel *daysModel;
    QJsonArray weekList;

    int days = 0;
    int weeks = 0;
    int firstDayOfWeek;
    QString errorMessage;

private:
    Q_DISABLE_COPY(CalendarPrivate)
};

CalendarPrivate::CalendarPrivate(Calendar *q)
    : types(Calendar::Holiday | Calendar::Event | Calendar::Todo | Calendar::Journal)
    , daysModel(new DaysModel(q))
    , firstDayOfWeek(QLocale::system().firstDayOfWeek())
{
    daysModel->setSourceData(&dayList);
}

Calendar::Calendar(QObject *parent)
    : QObject(parent)
    , d(new CalendarPrivate(this))
{
    //  m_dayHelper = new CalendarDayHelper(this);
    //   connect(m_dayHelper, SIGNAL(calendarChanged()), this, SLOT(updateData()));
    connect(this, &Calendar::monthNameChanged, this, &Calendar::monthChanged);
}

Calendar::~Calendar()
{
    delete d;
}

QDateTime Calendar::displayedDate() const
{
    return d->displayedDate.startOfDay();
}

void Calendar::setDisplayedDate(const QDate &dateTime)
{
    if (d->displayedDate == dateTime) {
        return;
    }

    const int oldMonth = d->displayedDate.month();
    const int oldYear = d->displayedDate.year();

    d->displayedDate = dateTime;

    //  m_dayHelper->setDate(m_displayedDate.year(), m_displayedDate.month());

    updateData();
    Q_EMIT displayedDateChanged();
    if (oldMonth != d->displayedDate.month()) {
        Q_EMIT monthNameChanged();
    }
    if (oldYear != d->displayedDate.year()) {
        Q_EMIT yearChanged();
    }
}

void Calendar::setDisplayedDate(const QDateTime &dateTime)
{
    setDisplayedDate(dateTime.date());
}

QDateTime Calendar::today() const
{
    return d->today.startOfDay();
}

void Calendar::setToday(const QDateTime &dateTime)
{
    QDate date = dateTime.date();
    if (date == d->today) {
        return;
    }

    d->today = date;
    if (d->displayedDate.isNull()) {
        resetToToday();
    } else {
        // the else is to prevent calling updateData() twice
        // if the resetToToday() was called
        updateData();
    }
    Q_EMIT todayChanged();
}

void Calendar::resetToToday()
{
    setDisplayedDate(d->today);
    updateData();
}

int Calendar::types() const
{
    return d->types;
}

void Calendar::setTypes(int types)
{
    if (d->types == static_cast<Types>(types)) {
        return;
    }

    //    d->m_types = static_cast<Types>(types);
    //    updateTypes();

    Q_EMIT typesChanged();
}

int Calendar::days()
{
    return d->days;
}

void Calendar::setDays(int days)
{
    if (d->days != days) {
        d->days = days;
        updateData();
        Q_EMIT daysChanged();
    }
}

int Calendar::weeks() const
{
    return d->weeks;
}

void Calendar::setWeeks(int weeks)
{
    if (d->weeks != weeks) {
        d->weeks = weeks;
        updateData();
        Q_EMIT weeksChanged();
    }
}

int Calendar::firstDayOfWeek() const
{
    // QML has Sunday as 0, so we need to accommodate here
    return d->firstDayOfWeek == 7 ? 0 : d->firstDayOfWeek;
}

void Calendar::setFirstDayOfWeek(int day)
{
    if (day > 7) {
        return;
    }

    if (d->firstDayOfWeek != day) {
        // QML has Sunday as 0, so we need to accommodate here
        // for QDate functions which have Sunday as 7
        if (day == 0) {
            d->firstDayOfWeek = 7;
        } else {
            d->firstDayOfWeek = day;
        }

        updateData();
        Q_EMIT firstDayOfWeekChanged();
    }
}

QString Calendar::errorMessage() const
{
    return d->errorMessage;
}

int Calendar::currentWeek() const
{
    QDate date(QDate::currentDate());
    return date.weekNumber();
}

QString Calendar::dayName(int weekday) const
{
    return QLocale::system().dayName(weekday, QLocale::ShortFormat);
}

QString Calendar::monthName() const
{
    // Simple QDate::longMonthName won't do the job as it
    // will return the month name using LC_DATE locale which is used
    // for date formatting etc. So for example, in en_US locale
    // and cs_CZ LC_DATE, it would return Czech month names while
    // it should return English ones. So here we force the LANG
    // locale and take the month name from that.
    //
    // See https://bugs.kde.org/show_bug.cgi?id=353715

    QLocale langLocale;

    if (QLocale().uiLanguages().length() > 0) {
        langLocale = QLocale(QLocale().uiLanguages().at(0));
    }
    return langLocale.standaloneMonthName(d->displayedDate.month());
}

int Calendar::year() const
{
    return d->displayedDate.year();
}

int Calendar::month() const
{
    return d->displayedDate.month();
}

QAbstractItemModel *Calendar::daysModel() const
{
    return d->daysModel;
}
QJsonArray Calendar::weeksModel() const
{
    return d->weekList;
}

void Calendar::updateData()
{
    if (d->days == 0 || d->weeks == 0) {
        return;
    }

    d->dayList.clear();
    d->weekList = QJsonArray();

    int totalDays = d->days * d->weeks;

    int daysBeforeCurrentMonth = 0;
    int daysAfterCurrentMonth = 0;

    QDate firstDay(d->displayedDate.year(), d->displayedDate.month(), 1);

    // If the first day is the same as the starting day then we add a complete row before it.
    if (d->firstDayOfWeek < firstDay.dayOfWeek()) {
        daysBeforeCurrentMonth = firstDay.dayOfWeek() - d->firstDayOfWeek;
    } else {
        daysBeforeCurrentMonth = days() - (d->firstDayOfWeek - firstDay.dayOfWeek());
    }

    int daysThusFar = daysBeforeCurrentMonth + d->displayedDate.daysInMonth();
    if (daysThusFar < totalDays) {
        daysAfterCurrentMonth = totalDays - daysThusFar;
    }

    if (daysBeforeCurrentMonth > 0) {
        QDate previousMonth = d->displayedDate.addMonths(-1);
        // QDate previousMonth(d->m_displayedDate.year(), d->m_displayedDate.month() - 1, 1);
        for (int i = 0; i < daysBeforeCurrentMonth; i++) {
            DayData day;
            day.isCurrent = false;
            day.dayNumber = previousMonth.daysInMonth() - (daysBeforeCurrentMonth - (i + 1));
            day.monthNumber = previousMonth.month();
            day.yearNumber = previousMonth.year();
            //      day.containsEventItems = false;
            d->dayList << day;
        }
    }

    for (int i = 0; i < d->displayedDate.daysInMonth(); i++) {
        DayData day;
        day.isCurrent = true;
        day.dayNumber = i + 1; // +1 to go form 0 based index to 1 based calendar dates
        //  day.containsEventItems = m_dayHelper->containsEventItems(i + 1);
        day.monthNumber = d->displayedDate.month();
        day.yearNumber = d->displayedDate.year();
        d->dayList << day;
    }

    if (daysAfterCurrentMonth > 0) {
        for (int i = 0; i < daysAfterCurrentMonth; i++) {
            DayData day;
            day.isCurrent = false;
            day.dayNumber = i + 1; // +1 to go form 0 based index to 1 based calendar dates
            //   day.containsEventItems = false;
            day.monthNumber = d->displayedDate.addMonths(1).month();
            day.yearNumber = d->displayedDate.addMonths(1).year();
            d->dayList << day;
        }
    }
    const int numOfDaysInCalendar = d->dayList.count();

    // Week numbers are always counted from Mondays
    // so find which index is Monday
    int mondayOffset = 0;
    if (!d->dayList.isEmpty()) {
        const DayData &data = d->dayList.at(0);
        QDate firstDay(data.yearNumber, data.monthNumber, data.dayNumber);
        // If the first day is not already Monday, get offset for Monday
        if (firstDay.dayOfWeek() != 1) {
            mondayOffset = 8 - firstDay.dayOfWeek();
        }
    }

    // Fill weeksModel with the week numbers
    for (int i = mondayOffset; i < numOfDaysInCalendar; i += 7) {
        const DayData &data = d->dayList.at(i);
        d->weekList.append(QDate(data.yearNumber, data.monthNumber, data.dayNumber).weekNumber());
    }
    Q_EMIT weeksModelChanged();
    d->daysModel->update();

    //    qDebug() << "---------------------------------------------------------------";
    //    qDebug() << "Date obj: " << d->m_displayedDate;
    //    qDebug() << "Month: " << d->m_displayedDate.month();
    //    qDebug() << "m_days: " << d->m_days;
    //    qDebug() << "m_weeks: " << d->m_weeks;
    //    qDebug() << "Days before this month: " << daysBeforeCurrentMonth;
    //    qDebug() << "Days after this month: " << daysAfterCurrentMonth;
    //    qDebug() << "Days in current month: " << d->m_displayedDate.daysInMonth();
    //    qDebug() << "d->m_dayList size: " << d->m_dayList.count();
    //    qDebug() << "---------------------------------------------------------------";
}

void Calendar::nextDecade()
{
    setDisplayedDate(d->displayedDate.addYears(10));
}

void Calendar::previousDecade()
{
    // Negative years don't make sense
    if (d->displayedDate.year() >= 10) {
        setDisplayedDate(d->displayedDate.addYears(-10));
    }
}

void Calendar::nextYear()
{
    setDisplayedDate(d->displayedDate.addYears(1));
}

void Calendar::previousYear()
{
    // Negative years don't make sense
    if (d->displayedDate.year() >= 1) {
        setDisplayedDate(d->displayedDate.addYears(-1));
    }
}

void Calendar::nextMonth()
{
    setDisplayedDate(d->displayedDate.addMonths(1));
}

void Calendar::previousMonth()
{
    setDisplayedDate(d->displayedDate.addMonths(-1));
}

void Calendar::goToMonth(int month)
{
    setDisplayedDate(QDate(d->displayedDate.year(), month, 1));
}

void Calendar::goToYear(int year)
{
    setDisplayedDate(QDate(year, d->displayedDate.month(), 1));
}
