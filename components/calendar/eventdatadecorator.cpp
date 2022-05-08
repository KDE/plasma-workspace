/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "eventdatadecorator.h"

#include <KLocalizedString>

EventDataDecorator::EventDataDecorator(const CalendarEvents::EventData &data, QObject *parent)
    : QObject(parent)
    , m_data(data)
{
}

QDateTime EventDataDecorator::startDateTime() const
{
    return m_data.startDateTime();
}

QDateTime EventDataDecorator::endDateTime() const
{
    return m_data.endDateTime();
}

bool EventDataDecorator::isAllDay() const
{
    return m_data.isAllDay();
}

bool EventDataDecorator::isMinor() const
{
    return m_data.isMinor();
}

QString EventDataDecorator::title() const
{
    return m_data.title();
}

QString EventDataDecorator::description() const
{
    return m_data.description();
}

QString EventDataDecorator::eventType() const
{
    switch (m_data.type()) {
    case CalendarEvents::EventData::Holiday:
        return i18nc("Agenda listview section title", "Holidays");
    case CalendarEvents::EventData::Event:
        return i18nc("Agenda listview section title", "Events");
    case CalendarEvents::EventData::Todo:
        return i18nc("Agenda listview section title", "Todo");
    }
    return i18nc("Means 'Other calendar items'", "Other");
}

QString EventDataDecorator::eventColor() const
{
    return m_data.eventColor();
}
