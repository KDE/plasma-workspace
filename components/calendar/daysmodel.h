/*
    SPDX-FileCopyrightText: 2013 Mark Gaiser <markg85@gmail.com>
    SPDX-FileCopyrightText: 2016 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DAYSMODEL_H
#define DAYSMODEL_H

#include <QAbstractItemModel>
#include <QPointer>

#include "daydata.h"
#include "eventpluginsmanager.h"
#include <CalendarEvents/CalendarEventsPlugin>

class DaysModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        isCurrent = Qt::UserRole + 1,
        // containsHolidayItems,
        containsEventItems,
        containsMajorEventItems,
        containsMinorEventItems,
        // containsTodoItems,
        // containsJournalItems,
        dayNumber,
        monthNumber,
        yearNumber,
        Events,
        EventColor,
        EventCount,
    };

    explicit DaysModel(QObject *parent = nullptr);
    ~DaysModel() override;
    void setSourceData(QList<DayData> *data);
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void setPluginsManager(QObject *manager);

    Q_INVOKABLE QList<QObject *> eventsForDate(const QDate &date);

    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void agendaUpdated(const QDate &updatedDate);

public Q_SLOTS:
    void update();

private Q_SLOTS:
    void onDataReady(const QMultiHash<QDate, CalendarEvents::EventData> &data);
    void onEventModified(const CalendarEvents::EventData &data);
    void onEventRemoved(const QString &uid);

private:
    QModelIndex indexForDate(const QDate &date);
    bool hasMajorEventAtDate(const QDate &date) const;
    bool hasMinorEventAtDate(const QDate &date) const;

    QPointer<EventPluginsManager> m_pluginsManager;
    QList<DayData> *m_data = nullptr;
    QList<QObject *> m_qmlData;
    QDate m_lastRequestedAgendaDate;
    QList<CalendarEvents::CalendarEventsPlugin *> m_eventPlugins;
    QMultiHash<QDate, CalendarEvents::EventData> m_eventsData;
    QDate m_lastRequestedEventsStartDate; // this is always this+42 days
    bool m_agendaNeedsUpdate;
};

#endif // DAYSMODEL_H
