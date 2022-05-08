/*
    SPDX-FileCopyrightText: 2013 Mark Gaiser <markg85@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "calendardata.h"

CalendarData::CalendarData(QObject *parent)
    : QObject(parent)
    , m_types(Holiday | Event | Todo | Journal)
{
    //   m_etmCalendar = new ETMCalendar();
    //   m_etmCalendar->setParent(this); //TODO: hit sergio

    // EntityTreeModel *model = m_etmCalendar->entityTreeModel();
    // model->setCollectionFetchStrategy(EntityTreeModel::InvisibleCollectionFetch);

    //  m_itemList = new EntityMimeTypeFilterModel(this);
    //  m_itemList->setSourceModel(model);

    // CalendarRoleProxyModel *roleModel = new CalendarRoleProxyModel(this);
    //  roleModel->setSourceModel(m_itemList);

    //  m_filteredList = new DateTimeRangeFilterModel(this);
    //  m_filteredList->setSourceModel(roleModel);

    //   updateTypes();
}

QDate CalendarData::startDate() const
{
    return m_startDate;
}

void CalendarData::setStartDate(const QDate &dateTime)
{
    if (m_startDate == dateTime) {
        return;
    }

    m_startDate = dateTime;
    //   m_filteredList->setStartDate(m_startDate);
    Q_EMIT startDateChanged();
}

QDate CalendarData::endDate() const
{
    return m_endDate;
}

void CalendarData::setEndDate(const QDate &dateTime)
{
    if (m_endDate == dateTime) {
        return;
    }

    m_endDate = dateTime;
    //  m_filteredList->setEndDate(m_endDate);
    Q_EMIT endDateChanged();
}

int CalendarData::types() const
{
    return m_types;
}

QString CalendarData::errorMessage() const
{
    return QString();
}

bool CalendarData::loading() const
{
    return false;
}
