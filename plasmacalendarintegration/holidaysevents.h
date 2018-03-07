/*
    Copyright (C) 2015 Martin Klapetek <mklapetek@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#ifndef HOLIDAYSEVENTSPLUGIN_H
#define HOLIDAYSEVENTSPLUGIN_H

#include <QObject>
#include <CalendarEvents/CalendarEventsPlugin>

#include <KHolidays/HolidayRegion>
#include <KSharedConfig>

class HolidaysEventsPlugin : public CalendarEvents::CalendarEventsPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.CalendarEventsPlugin" FILE "holidayeventsplugin.json")
    Q_INTERFACES(CalendarEvents::CalendarEventsPlugin)

public:
    explicit HolidaysEventsPlugin(QObject *parent = nullptr);
    ~HolidaysEventsPlugin() override;

    void loadEventsForDateRange(const QDate &startDate, const QDate &endDate) override;

private:
    QDate m_lastStartDate;
    QDate m_lastEndDate;
    QList<KHolidays::HolidayRegion*> m_regions;
    QMultiHash<QDate, CalendarEvents::EventData> m_lastData;
    KSharedConfig::Ptr m_config;
};

#endif
