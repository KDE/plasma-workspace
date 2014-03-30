/*
    Copyright (c) 2010 Frederik Gladhorn <gladhorn@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef EVENTDATACONTAINER_H
#define EVENTDATACONTAINER_H

#include <KDateTime>
#include <Akonadi/Calendar/ETMCalendar>
#include <KCalCore/Incidence>
#include <plasma/datacontainer.h>

class EventDataContainer : public Plasma::DataContainer
{
    Q_OBJECT
public:
    EventDataContainer(const Akonadi::ETMCalendar::Ptr &calendar, const QString& name, const KDateTime& start, const KDateTime& end, QObject* parent = 0);

public Q_SLOTS:
    // update the list of incidents
    void updateData();

private:
    void updateEventData();
    void updateTodoData();
    void updateJournalData();
    void populateIncidenceData(const KCalCore::Incidence::Ptr &incidence, Plasma::DataEngine::Data &incidenceData);

    Akonadi::ETMCalendar::Ptr m_calendar;
    QString m_name;
    KDateTime m_startDate;
    KDateTime m_endDate;
};

#endif
