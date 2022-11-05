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

class DaysModelPrivate
{
public:
    explicit DaysModelPrivate();

    QList<DayData> *data = nullptr;
    QList<QObject *> qmlData;
    QMultiHash<QDate, CalendarEvents::EventData> eventsData;
    QHash<QDate /* Gregorian */, QDate> alternateDatesData;
    QHash<QDate, CalendarEvents::CalendarEventsPlugin::SubLabel> subLabelsData;

    QDate lastRequestedAgendaDate;
    bool agendaNeedsUpdate = false;

    // QML Ownership
    EventPluginsManager *pluginsManager = nullptr;
};

DaysModelPrivate::DaysModelPrivate()
{
}

DaysModel::DaysModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d(new DaysModelPrivate)
{
}

DaysModel::~DaysModel()
{
    delete d;
}

void DaysModel::setSourceData(QList<DayData> *data)
{
    if (d->data != data) {
        beginResetModel();
        d->data = data;
        endResetModel();
    }
}

int DaysModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        // day count
        if (d->data->size() <= 0) {
            return 0;
        } else {
            return d->data->size();
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
        const DayData &currentData = d->data->at(row);
        const QDate currentDate(currentData.yearNumber, currentData.monthNumber, currentData.dayNumber);

        switch (role) {
        case isCurrent:
            return currentData.isCurrent;
        case containsEventItems:
            return d->eventsData.contains(currentDate);
        case Events:
            return QVariant::fromValue(d->eventsData.values(currentDate));
        case EventCount:
            return d->eventsData.values(currentDate).count();
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
        default:
            break;
        }

        if (d->alternateDatesData.count(currentDate)) {
            switch (role) {
            case AlternateYearNumber:
                return d->alternateDatesData.value(currentDate).year();
            case AlternateMonthNumber:
                return d->alternateDatesData.value(currentDate).month();
            case AlternateDayNumber:
                return d->alternateDatesData.value(currentDate).day();
            default:
                break;
            }
        }

        if (d->subLabelsData.count(currentDate)) {
            switch (role) {
            case SubLabel:
                return d->subLabelsData.value(currentDate).label;
            case SubYearLabel:
                return d->subLabelsData.value(currentDate).yearLabel;
            case SubMonthLabel:
                return d->subLabelsData.value(currentDate).monthLabel;
            case SubDayLabel:
                return d->subLabelsData.value(currentDate).dayLabel;
            default:
                break;
            }
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
    if (d->data->size() <= 0) {
        return;
    }

    // We need to reset the model since m_data has already been changed here
    // and we can't remove the events manually with beginRemoveRows() since
    // we don't know where the old events were located.
    beginResetModel();
    d->eventsData.clear();
    d->alternateDatesData.clear();
    d->subLabelsData.clear();
    endResetModel();

    if (d->pluginsManager) {
        const QDate modelFirstDay(d->data->at(0).yearNumber, d->data->at(0).monthNumber, d->data->at(0).dayNumber);
        const auto plugins = d->pluginsManager->plugins();
        for (CalendarEvents::CalendarEventsPlugin *eventsPlugin : plugins) {
            eventsPlugin->loadEventsForDateRange(modelFirstDay, modelFirstDay.addDays(42));
        }
    }

    // We always have 42 items (or weeks * num of days in week) so we only have to tell the view that the data changed.
    Q_EMIT dataChanged(index(0, 0), index(d->data->count() - 1, 0));
}

void DaysModel::onDataReady(const QMultiHash<QDate, CalendarEvents::EventData> &data)
{
    d->eventsData.reserve(d->eventsData.size() + data.size());
    for (int i = 0; i < d->data->count(); i++) {
        const DayData &currentData = d->data->at(i);
        const QDate currentDate(currentData.yearNumber, currentData.monthNumber, currentData.dayNumber);
        if (!data.values(currentDate).isEmpty()) {
            // Make sure we don't display more than maxEventDisplayed events.
            const int currentCount = d->eventsData.values(currentDate).count();
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
                d->eventsData.insert(currentDate, dataDay);
            }
            endInsertRows();
        }
    }

    if (data.contains(QDate::currentDate())) {
        d->agendaNeedsUpdate = true;
    }

    // only the containsEventItems roles may have changed
    Q_EMIT dataChanged(index(0, 0), index(d->data->count() - 1, 0), {containsEventItems, containsMajorEventItems, containsMinorEventItems, Events, EventCount});

    Q_EMIT agendaUpdated(QDate::currentDate());
}

void DaysModel::onEventModified(const CalendarEvents::EventData &data)
{
    QList<QDate> updatesList;
    auto i = d->eventsData.begin();
    while (i != d->eventsData.end()) {
        if (i->uid() == data.uid()) {
            *i = data;
            updatesList << i.key();
        }

        ++i;
    }

    if (!updatesList.isEmpty()) {
        d->agendaNeedsUpdate = true;
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
    auto i = d->eventsData.begin();
    while (i != d->eventsData.end()) {
        if (i->uid() == uid) {
            updatesList << i.key();
            i = d->eventsData.erase(i);
        } else {
            ++i;
        }
    }

    if (!updatesList.isEmpty()) {
        d->agendaNeedsUpdate = true;
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

void DaysModel::onAlternateDateReady(const QHash<QDate, QDate> &data)
{
    d->alternateDatesData.reserve(d->alternateDatesData.size() + data.size());
    for (int i = 0; i < d->data->count(); i++) {
        const DayData &currentData = d->data->at(i);
        const QDate currentDate(currentData.yearNumber, currentData.monthNumber, currentData.dayNumber);
        if (data.count(currentDate) == 0) {
            continue;
        }
        // Add an alternate date
        d->alternateDatesData.insert(currentDate, data.value(currentDate));
    }

    Q_EMIT dataChanged(index(0, 0), index(d->data->count() - 1, 0), {AlternateYearNumber, AlternateMonthNumber, AlternateDayNumber});
}

void DaysModel::onSubLabelReady(const QHash<QDate, CalendarEvents::CalendarEventsPlugin::SubLabel> &data)
{
    d->subLabelsData.reserve(d->subLabelsData.size() + data.size());
    for (int i = 0; i < d->data->count(); i++) {
        const DayData &currentData = d->data->at(i);
        const QDate currentDate(currentData.yearNumber, currentData.monthNumber, currentData.dayNumber);
        if (data.count(currentDate) == 0) {
            continue;
        }
        // Add/Overwrite a sub-label based on priority
        if (const auto &value = data.value(currentDate);
            !d->subLabelsData.count(currentDate) || (d->subLabelsData.count(currentDate) && value.priority > d->subLabelsData.value(currentDate).priority)) {
            d->subLabelsData.insert(currentDate, value);
        }
    }

    Q_EMIT dataChanged(index(0, 0), index(d->data->count() - 1, 0), {SubLabel, SubYearLabel, SubMonthLabel, SubDayLabel});
}

QList<QObject *> DaysModel::eventsForDate(const QDate &date)
{
    if (d->lastRequestedAgendaDate == date && !d->agendaNeedsUpdate) {
        return d->qmlData;
    }

    d->lastRequestedAgendaDate = date;
    qDeleteAll(d->qmlData);
    d->qmlData.clear();

    QList<CalendarEvents::EventData> events = d->eventsData.values(date);
    d->qmlData.reserve(events.size());

    // sort events by their time and type
    std::sort(events.begin(), events.end(), [](const CalendarEvents::EventData &a, const CalendarEvents::EventData &b) {
        return b.type() > a.type() || b.startDateTime() > a.startDateTime();
    });

    for (const CalendarEvents::EventData &event : std::as_const(events)) {
        d->qmlData << new EventDataDecorator(event, this);
    }

    d->agendaNeedsUpdate = false;
    return d->qmlData;
}

QModelIndex DaysModel::indexForDate(const QDate &date)
{
    if (!d->data) {
        return QModelIndex();
    }

    const DayData &firstDay = d->data->at(0);
    const QDate firstDate(firstDay.yearNumber, firstDay.monthNumber, firstDay.dayNumber);

    qint64 daysTo = firstDate.daysTo(date);

    return createIndex(daysTo, 0);
}

bool DaysModel::hasMajorEventAtDate(const QDate &date) const
{
    auto it = d->eventsData.find(date);
    while (it != d->eventsData.end() && it.key() == date) {
        if (!it.value().isMinor()) {
            return true;
        }
        ++it;
    }
    return false;
}

bool DaysModel::hasMinorEventAtDate(const QDate &date) const
{
    auto it = d->eventsData.find(date);
    while (it != d->eventsData.end() && it.key() == date) {
        if (it.value().isMinor()) {
            return true;
        }
        ++it;
    }
    return false;
}

void DaysModel::setPluginsManager(QObject *manager)
{
    if (d->pluginsManager) {
        disconnect(d->pluginsManager, nullptr, this, nullptr);
    }

    EventPluginsManager *m = qobject_cast<EventPluginsManager *>(manager);

    if (!m) {
        return;
    }

    d->pluginsManager = m;

    connect(d->pluginsManager, &EventPluginsManager::dataReady, this, &DaysModel::onDataReady);
    connect(d->pluginsManager, &EventPluginsManager::eventModified, this, &DaysModel::onEventModified);
    connect(d->pluginsManager, &EventPluginsManager::eventRemoved, this, &DaysModel::onEventRemoved);
    connect(d->pluginsManager, &EventPluginsManager::alternateDateReady, this, &DaysModel::onAlternateDateReady);
    connect(d->pluginsManager, &EventPluginsManager::subLabelReady, this, &DaysModel::onSubLabelReady);
    connect(d->pluginsManager, &EventPluginsManager::pluginsChanged, this, &DaysModel::update);

    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
}

QHash<int, QByteArray> DaysModel::roleNames() const
{
    return {
        {isCurrent, "isCurrent"},
        {containsEventItems, "containsEventItems"},
        {containsMajorEventItems, "containsMajorEventItems"},
        {containsMinorEventItems, "containsMinorEventItems"},
        {dayNumber, "dayNumber"},
        {monthNumber, "monthNumber"},
        {yearNumber, "yearNumber"},
        {EventColor, "eventColor"},
        {EventCount, "eventCount"},
        {Events, "events"},
        {AlternateYearNumber, "alternateYearNumber"},
        {AlternateMonthNumber, "alternateMonthNumber"},
        {AlternateDayNumber, "alternateDayNumber"},
        {SubLabel, "subLabel"},
        {SubYearLabel, "subYearLabel"},
        {SubMonthLabel, "subMonthLabel"},
        {SubDayLabel, "subDayLabel"},
    };
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
