/*
    SPDX-FileCopyrightText: 2013 Mark Gaiser <markg85@gmail.com>
    SPDX-FileCopyrightText: 2016 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "daysmodel.h"
#include "eventdatadecorator.h"

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QMetaObject>

constexpr int maxEventDisplayed = 5;

DaysModel::DaysModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_pluginsManager(nullptr)
    , m_lastRequestedEventsStartDate(QDate())
    , m_agendaNeedsUpdate(false)
{
}

DaysModel::~DaysModel()
{
    qDeleteAll(m_eventPlugins);
}

void DaysModel::setSourceData(QList<DayData> *data)
{
    if (m_data != data) {
        beginResetModel();
        m_data = data;
        endResetModel();
    }
}

int DaysModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        // day count
        if (m_data->size() <= 0) {
            return 0;
        } else {
            return m_data->size();
        }
    } else {
        // event count
        const auto &eventDatas = data(parent, Roles::Events).value<QList<CalendarEvents::EventData>>();
        Q_ASSERT(eventDatas.count() <= maxEventDisplayed);
        return eventDatas.count();
    }
}

int DaysModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant DaysModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const int row = index.row();

    if (!index.parent().isValid()) {
        // Fetch days in month
        const DayData &currentData = m_data->at(row);
        const QDate currentDate(currentData.yearNumber, currentData.monthNumber, currentData.dayNumber);

        switch (role) {
        case isCurrent:
            return currentData.isCurrent;
        case containsEventItems:
            return m_eventsData.contains(currentDate);
        case Events:
            return QVariant::fromValue(m_eventsData.values(currentDate));
        case EventCount:
            return m_eventsData.values(currentDate).count();
        case containsMajorEventItems:
            return hasMajorEventAtDate(currentDate);
        case containsMinorEventItems:
            return hasMinorEventAtDate(currentDate);
        case dayNumber:
            return currentData.dayNumber;
        case monthNumber:
            return currentData.monthNumber;
        case yearNumber:
            return currentData.yearNumber;
        }
    } else {
        // Fetch event in day
        const auto &eventDatas = data(index.parent(), Roles::Events).value<QList<CalendarEvents::EventData>>();
        if (eventDatas.count() < row) {
            return {};
        }

        const auto &eventData = eventDatas[row];
        switch (role) {
        case EventColor:
            return eventData.eventColor();
        }
    }
    return {};
}

void DaysModel::update()
{
    if (m_data->size() <= 0) {
        return;
    }

    // We need to reset the model since m_data has already been changed here
    // and we can't remove the events manually with beginRemoveRows() since
    // we don't know where the old events were located.
    beginResetModel();
    m_eventsData.clear();
    endResetModel();

    const QDate modelFirstDay(m_data->at(0).yearNumber, m_data->at(0).monthNumber, m_data->at(0).dayNumber);

    if (m_pluginsManager) {
        const auto plugins = m_pluginsManager->plugins();
        for (CalendarEvents::CalendarEventsPlugin *eventsPlugin : plugins) {
            eventsPlugin->loadEventsForDateRange(modelFirstDay, modelFirstDay.addDays(42));
        }
    }

    // We always have 42 items (or weeks * num of days in week) so we only have to tell the view that the data changed.
    Q_EMIT dataChanged(index(0, 0), index(m_data->count() - 1, 0));
}

void DaysModel::onDataReady(const QMultiHash<QDate, CalendarEvents::EventData> &data)
{
    m_eventsData.reserve(m_eventsData.size() + data.size());
    for (int i = 0; i < m_data->count(); i++) {
        const DayData &currentData = m_data->at(i);
        const QDate currentDate(currentData.yearNumber, currentData.monthNumber, currentData.dayNumber);
        if (!data.values(currentDate).isEmpty()) {
            // Make sure we don't display more than maxEventDisplayed events.
            const int currentCount = m_eventsData.values(currentDate).count();
            if (currentCount >= maxEventDisplayed) {
                break;
            }

            const int addedEventCount = std::min<int>(currentCount + data.values(currentDate).count(), maxEventDisplayed) - currentCount;

            // Add event
            beginInsertRows(index(i, 0), 0, addedEventCount - 1);
            int stopCounter = currentCount;
            for (const auto &dataDay : data.values(currentDate)) {
                if (stopCounter >= maxEventDisplayed) {
                    break;
                }
                stopCounter++;
                m_eventsData.insert(currentDate, dataDay);
            }
            endInsertRows();
        }
    }

    if (data.contains(QDate::currentDate())) {
        m_agendaNeedsUpdate = true;
    }

    // only the containsEventItems roles may have changed
    Q_EMIT dataChanged(index(0, 0),
                       index(d->m_data->count() - 1, 0),
                       {containsEventItems, containsMajorEventItems, containsMinorEventItems, Events, EventCount});

    Q_EMIT agendaUpdated(QDate::currentDate());
}

void DaysModel::onEventModified(const CalendarEvents::EventData &data)
{
    QList<QDate> updatesList;
    auto i = m_eventsData.begin();
    while (i != m_eventsData.end()) {
        if (i->uid() == data.uid()) {
            *i = data;
            updatesList << i.key();
        }

        ++i;
    }

    if (!updatesList.isEmpty()) {
        m_agendaNeedsUpdate = true;
    }

    for (const QDate date : std::as_const(updatesList)) {
        const QModelIndex changedIndex = indexForDate(date);
        if (changedIndex.isValid()) {
            Q_EMIT dataChanged(changedIndex, changedIndex, {containsEventItems, containsMajorEventItems, containsMinorEventItems, EventColor});
        }
        Q_EMIT agendaUpdated(date);
    }
}

void DaysModel::onEventRemoved(const QString &uid)
{
    // HACK We should update the model with beginRemoveRows instead of
    // using beginResetModel() since this creates a small visual glitches
    // if an event is removed in Korganizer and the calendar is open.
    // Using beginRemoveRows instead we make the code a lot more complex
    // and if not done correctly will introduce bugs.
    beginResetModel();
    QList<QDate> updatesList;
    auto i = m_eventsData.begin();
    while (i != m_eventsData.end()) {
        if (i->uid() == uid) {
            updatesList << i.key();
            i = m_eventsData.erase(i);
        } else {
            ++i;
        }
    }

    if (!updatesList.isEmpty()) {
        m_agendaNeedsUpdate = true;
    }

    for (const QDate date : std::as_const(updatesList)) {
        const QModelIndex changedIndex = indexForDate(date);
        if (changedIndex.isValid()) {
            Q_EMIT dataChanged(changedIndex, changedIndex, {containsEventItems, containsMajorEventItems, containsMinorEventItems});
        }

        Q_EMIT agendaUpdated(date);
    }
    endResetModel();
}

QList<QObject *> DaysModel::eventsForDate(const QDate &date)
{
    if (m_lastRequestedAgendaDate == date && !m_agendaNeedsUpdate) {
        return m_qmlData;
    }

    m_lastRequestedAgendaDate = date;
    qDeleteAll(m_qmlData);
    m_qmlData.clear();

    QList<CalendarEvents::EventData> events = m_eventsData.values(date);
    m_qmlData.reserve(events.size());

    // sort events by their time and type
    std::sort(events.begin(), events.end(), [](const CalendarEvents::EventData &a, const CalendarEvents::EventData &b) {
        return b.type() > a.type() || b.startDateTime() > a.startDateTime();
    });

    for (const CalendarEvents::EventData &event : std::as_const(events)) {
        m_qmlData << new EventDataDecorator(event, this);
    }

    m_agendaNeedsUpdate = false;
    return m_qmlData;
}

QModelIndex DaysModel::indexForDate(const QDate &date)
{
    if (!m_data) {
        return QModelIndex();
    }

    const DayData &firstDay = m_data->at(0);
    const QDate firstDate(firstDay.yearNumber, firstDay.monthNumber, firstDay.dayNumber);

    qint64 daysTo = firstDate.daysTo(date);

    return createIndex(daysTo, 0);
}

bool DaysModel::hasMajorEventAtDate(const QDate &date) const
{
    auto it = m_eventsData.find(date);
    while (it != m_eventsData.end() && it.key() == date) {
        if (!it.value().isMinor()) {
            return true;
        }
        ++it;
    }
    return false;
}

bool DaysModel::hasMinorEventAtDate(const QDate &date) const
{
    auto it = m_eventsData.find(date);
    while (it != m_eventsData.end() && it.key() == date) {
        if (it.value().isMinor()) {
            return true;
        }
        ++it;
    }
    return false;
}

void DaysModel::setPluginsManager(QObject *manager)
{
    EventPluginsManager *m = qobject_cast<EventPluginsManager *>(manager);

    if (!m) {
        return;
    }

    if (m_pluginsManager) {
        m_pluginsManager->deleteLater();
        m_pluginsManager = nullptr;
    }

    m_pluginsManager = m;

    connect(m_pluginsManager, &EventPluginsManager::dataReady, this, &DaysModel::onDataReady);
    connect(m_pluginsManager, &EventPluginsManager::eventModified, this, &DaysModel::onEventModified);
    connect(m_pluginsManager, &EventPluginsManager::eventRemoved, this, &DaysModel::onEventRemoved);
    connect(m_pluginsManager, &EventPluginsManager::pluginsChanged, this, &DaysModel::update);

    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
}

QHash<int, QByteArray> DaysModel::roleNames() const
{
    return {{isCurrent, "isCurrent"},
            {containsEventItems, "containsEventItems"},
            {containsMajorEventItems, "containsMajorEventItems"},
            {containsMinorEventItems, "containsMinorEventItems"},
            {dayNumber, "dayNumber"},
            {monthNumber, "monthNumber"},
            {yearNumber, "yearNumber"},
            {EventColor, "eventColor"},
            {EventCount, "eventCount"},
            {Events, "events"}};
}

QModelIndex DaysModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return createIndex(row, column, (intptr_t)parent.row());
    }
    return createIndex(row, column, nullptr);
}

QModelIndex DaysModel::parent(const QModelIndex &child) const
{
    if (child.internalId()) {
        return createIndex(child.internalId(), 0, nullptr);
    }
    return QModelIndex();
}

Q_DECLARE_METATYPE(CalendarEvents::EventData)
