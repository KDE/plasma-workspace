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

#include "holidaysevents.h"

#include <KConfigGroup>
#include <QDebug>

HolidaysEventsPlugin::HolidaysEventsPlugin(QObject *parent)
    : CalendarEvents::CalendarEventsPlugin(parent)
{
    KSharedConfig::Ptr m_config = KSharedConfig::openConfig(QStringLiteral("plasma_calendar_holiday_regions"));
    const KConfigGroup regionsConfig = m_config->group("General");
    QStringList regionCodes = regionsConfig.readEntry("selectedRegions", QStringList());
    regionCodes.removeDuplicates();

    // If the config does not have any region stored
    // add the default one
    if (regionCodes.isEmpty()) {
        regionCodes << KHolidays::HolidayRegion::defaultRegionCode();
    }

    Q_FOREACH (const QString &region, regionCodes) {
        m_regions << new KHolidays::HolidayRegion(region);
    }
}

HolidaysEventsPlugin::~HolidaysEventsPlugin()
{
    qDeleteAll(m_regions);
}

void HolidaysEventsPlugin::loadEventsForDateRange(const QDate &startDate, const QDate &endDate)
{
    if (m_lastStartDate == startDate && m_lastEndDate == endDate) {
        Q_EMIT dataReady(m_lastData);
        return;
    }

    m_lastData.clear();
    QMultiHash<QDate, CalendarEvents::EventData> data;

    Q_FOREACH (KHolidays::HolidayRegion *region, m_regions) {
        KHolidays::Holiday::List holidays = region->holidays(startDate, endDate);

        Q_FOREACH (const KHolidays::Holiday &holiday, holidays) {
            CalendarEvents::EventData eventData;
            eventData.setStartDateTime(QDateTime(holiday.observedStartDate()));
            eventData.setEndDateTime(QDateTime(holiday.observedEndDate()));
            eventData.setIsAllDay(true);
            eventData.setTitle(holiday.name());
            eventData.setEventType(CalendarEvents::EventData::Holiday);
            eventData.setIsMinor(false);

            // make sure to add events spanning multiple days to all of them
            for (QDate d = holiday.observedStartDate(); d <= holiday.observedEndDate(); d = d.addDays(1)) {
                data.insert(d, eventData);
            }
        }
    }

    m_lastStartDate = startDate;
    m_lastEndDate = endDate;
    m_lastData = data;

    qDebug() << data.size();

    Q_EMIT dataReady(data);
}
