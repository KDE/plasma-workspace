/*
    SPDX-FileCopyrightText: 2013 Mark Gaiser <markg85@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CALENDARDATA_H
#define CALENDARDATA_H

#include <QDate>
#include <QFlags>
#include <QObject>

class QAbstractItemModel;

class CalendarData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate NOTIFY startDateChanged)
    Q_PROPERTY(QDate endDate READ endDate WRITE setEndDate NOTIFY endDateChanged)
    //  Q_PROPERTY(int types READ types WRITE setTypes NOTIFY typesChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    //  Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)

public:
    enum Type {
        Holiday = 1,
        Event = 2,
        Todo = 4,
        Journal = 8,
    };
    Q_ENUM(Type)
    Q_DECLARE_FLAGS(Types, Type)

    explicit CalendarData(QObject *parent = nullptr);

Q_SIGNALS:
    void startDateChanged();
    void endDateChanged();
    void typesChanged();
    void errorMessageChanged();
    void loadingChanged();

private:
    QDate startDate() const;
    void setStartDate(const QDate &dateTime);
    QDate endDate() const;
    void setEndDate(const QDate &dateTime);
    int types() const;
    // void setTypes(int types);
    QString errorMessage() const;
    bool loading() const;
    //  QAbstractItemModel* model() const;

    // void updateTypes();

    QDate m_startDate;
    QDate m_endDate;
    Types m_types;

    // Akonadi::ETMCalendar *m_etmCalendar;
    // Akonadi::EntityMimeTypeFilterModel *m_itemList;
    // DateTimeRangeFilterModel *m_filteredList;
};

#endif // CALENDARDATA_H
