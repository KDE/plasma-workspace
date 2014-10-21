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

#include "eventdatacontainer.h"

#include <KSystemTimeZones>

#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <KCalCore/Journal>
#include <KCalUtils/Stringify>

using namespace Akonadi;

EventDataContainer::EventDataContainer(const Akonadi::ETMCalendar::Ptr &calendar, const QString& name, const KDateTime& start, const KDateTime& end, QObject* parent)
                  : Plasma::DataContainer(parent),
                    m_calendar(calendar),
                    m_name(name),
                    m_startDate(start),
                    m_endDate(end)
{
    // name under which this dataEngine source appears
    setObjectName(name);

    // Connect directly to the calendar for now
    connect(calendar.data(), SIGNAL(calendarChanged()), this, SLOT(updateData()));

    // create the initial data
    updateData();
}

void EventDataContainer::updateData()
{
    removeAllData();
    updateEventData();
    updateTodoData();
    updateJournalData();
    checkForUpdate();
}

void EventDataContainer::updateEventData()
{
    KCalCore::Event::List events = m_calendar->events(m_startDate.date(), m_endDate.date(), m_calendar->timeSpec());

    foreach (const KCalCore::Event::Ptr &event, events) {
        Plasma::DataEngine::Data eventData;
        populateIncidenceData(event, eventData);

        // Event specific fields
        eventData["EventMultiDay"] = event->allDay();
        eventData["EventHasEndDate"] = event->hasEndDate();
        if (event->transparency() == KCalCore::Event::Opaque) {
            eventData["EventTransparency"] = "Opaque";
        } else if (event->transparency() == KCalCore::Event::Transparent) {
            eventData["EventTransparency"] = "Transparent";
        } else {
            eventData["EventTransparency"] = "Unknown";
        }

        setData(event->uid(), eventData);
    }
}

void EventDataContainer::updateTodoData()
{
    QDate todoDate = m_startDate.date();
    while(todoDate <= m_endDate.date()) {
        KCalCore::Todo::List todos = m_calendar->todos(todoDate);

        foreach (const KCalCore::Todo::Ptr &todo, todos) {
            Plasma::DataEngine::Data todoData;
            populateIncidenceData(todo, todoData);

            QVariant var;
            // Todo specific fields
            todoData["TodoHasStartDate"] = todo->hasStartDate();
            todoData["TodoIsOpenEnded"] = todo->isOpenEnded();
            todoData["TodoHasDueDate"] = todo->hasDueDate();
            var.setValue(todo->dtDue().toTimeSpec(m_calendar->timeSpec()));
            todoData["TodoDueDate"] = var;
            todoData["TodoIsCompleted"] = todo->isCompleted();
            todoData["TodoIsInProgress"] = todo->isInProgress(false); // ???
            todoData["TodoIsNotStarted"] = todo->isNotStarted(false); // ???
            todoData["TodoPercentComplete"] = todo->percentComplete();
            todoData["TodoHasCompletedDate"] = todo->hasCompletedDate();
            var.setValue(todo->completed().toTimeSpec(m_calendar->timeSpec()));
            todoData["TodoCompletedDate"] = var;

            setData(todo->uid(), todoData);
        }

        todoDate = todoDate.addDays(1);
    }
}

void EventDataContainer::updateJournalData()
{
    QDate journalDate = m_startDate.date();
    while(journalDate <= m_endDate.date()) {
        KCalCore::Journal::List journals = m_calendar->journals(journalDate);

        foreach (const KCalCore::Journal::Ptr &journal, journals) {
            Plasma::DataEngine::Data journalData;
            populateIncidenceData(journal, journalData);

            // No Journal specific fields
            setData(journal->uid(), journalData);
        }

        journalDate = journalDate.addDays(1);
    }
}

void EventDataContainer::populateIncidenceData(const KCalCore::Incidence::Ptr &incidence, Plasma::DataEngine::Data &incidenceData)
{
    QVariant var;
    incidenceData["UID"] = incidence->uid();
    incidenceData["Type"] = incidence->typeStr();
    incidenceData["Summary"] = incidence->summary();
    incidenceData["Description"] = incidence->description();
    incidenceData["Comments"] = incidence->comments();
    incidenceData["Location"] = incidence->location();
    incidenceData["OrganizerName"] = incidence->organizer()->name();
    incidenceData["OrganizerEmail"] = incidence->organizer()->email();
    incidenceData["Priority"] = incidence->priority();
    var.setValue(incidence->dtStart().toTimeSpec(m_calendar->timeSpec()));
    incidenceData["StartDate"] = var;

    KCalCore::Event* event = dynamic_cast<KCalCore::Event*>(incidence.data());
    if (event) {
        var.setValue(event->dtEnd().toTimeSpec(m_calendar->timeSpec()));
        incidenceData["EndDate"] = var;
    }
    // Build the Occurance Index, this lists all occurences of the Incidence in the required range
    // Single occurance events just repeat the standard start/end dates
    // Recurring Events use each recurrence start/end date
    // The OccurenceUid is redundant, but it makes it easy for clients to just take() the data structure intact as a separate index
    QList<QVariant> occurences;
    // Build the recurrence list of start dates for recurring incidences only
    QList<QVariant> recurrences;
    if (incidence->recurs()) {
        KCalCore::DateTimeList recurList = incidence->recurrence()->timesInInterval(m_startDate.toTimeSpec(m_calendar->timeSpec()), m_endDate.toTimeSpec(m_calendar->timeSpec()));
        foreach(const KDateTime &recurDateTime, recurList) {
            var.setValue(recurDateTime.toTimeSpec(m_calendar->timeSpec()));
            recurrences.append(var);
            Plasma::DataEngine::Data occurence;
            occurence.insert("OccurrenceUid", incidence->uid());
            occurence.insert("OccurrenceStartDate", var);
            var.setValue(incidence->endDateForStart(recurDateTime).toTimeSpec(m_calendar->timeSpec()));
            occurence.insert("OccurrenceEndDate", var);
            occurences.append(QVariant(occurence));
        }
    } else {
        Plasma::DataEngine::Data occurence;
        occurence.insert("OccurrenceUid", incidence->uid());
        var.setValue(incidence->dtStart().toTimeSpec(m_calendar->timeSpec()));
        occurence.insert("OccurrenceStartDate", var);
	if (event) {
            var.setValue(event->dtEnd().toTimeSpec(m_calendar->timeSpec()));
            occurence.insert("OccurrenceEndDate", var);
        }
        occurences.append(QVariant(occurence));
    }
    incidenceData["RecurrenceDates"] = QVariant(recurrences);
    incidenceData["Occurrences"] = QVariant(occurences);
    if (incidence->status() == KCalCore::Incidence::StatusNone) {
        incidenceData["Status"] = "None";
    } else if (incidence->status() == KCalCore::Incidence::StatusTentative) {
        incidenceData["Status"] = "Tentative";
    } else if (incidence->status() == KCalCore::Incidence::StatusConfirmed) {
        incidenceData["Status"] = "Confirmed";
    } else if (incidence->status() == KCalCore::Incidence::StatusDraft) {
        incidenceData["Status"] = "Draft";
    } else if (incidence->status() == KCalCore::Incidence::StatusFinal) {
        incidenceData["Status"] = "Final";
    } else if (incidence->status() == KCalCore::Incidence::StatusCompleted) {
        incidenceData["Status"] = "Completed";
    } else if (incidence->status() == KCalCore::Incidence::StatusInProcess) {
        incidenceData["Status"] = "InProcess";
    } else if (incidence->status() == KCalCore::Incidence::StatusCanceled) {
        incidenceData["Status"] = "Cancelled";
    } else if (incidence->status() == KCalCore::Incidence::StatusNeedsAction) {
        incidenceData["Status"] = "NeedsAction";
    } else if (incidence->status() == KCalCore::Incidence::StatusX) {
        incidenceData["Status"] = "NonStandard";
    } else {
        incidenceData["Status"] = "Unknown";
    }
    incidenceData["StatusName"] = KCalUtils::Stringify::incidenceStatus( incidence->status() );

    if (incidence->secrecy() == KCalCore::Incidence::SecrecyPublic) {
        incidenceData["Secrecy"] = "Public";
    } else if (incidence->secrecy() == KCalCore::Incidence::SecrecyPrivate) {
        incidenceData["Secrecy"] = "Private";
    } else if (incidence->secrecy() == KCalCore::Incidence::SecrecyConfidential) {
        incidenceData["Secrecy"] = "Confidential";
    } else {
        incidenceData["Secrecy"] = "Unknown";
    }
    incidenceData["SecrecyName"] = KCalUtils::Stringify::secrecyName( incidence->secrecy() );
    incidenceData["Recurs"] = incidence->recurs();
    incidenceData["AllDay"] = incidence->allDay();
    incidenceData["Categories"] = incidence->categories();
    incidenceData["Resources"] = incidence->resources();
    incidenceData["DurationDays"] = incidence->duration().asDays();
    incidenceData["DurationSeconds"] = incidence->duration().asSeconds();

    //TODO Attendees
    //TODO Attachments
    //TODO Relations
    //TODO Alarms
    //TODO Custom Properties
    //TODO Lat/Lon
    //TODO Collection/Source
}


