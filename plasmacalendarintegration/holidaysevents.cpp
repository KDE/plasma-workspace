/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "holidaysevents.h"

#include <KConfigGroup>
#include <QDebug>

HolidaysEventsPlugin::HolidaysEventsPlugin(QObject *parent)
    : CalendarEvents::CalendarEventsPlugin(parent)
{
    KSharedConfig::Ptr m_config = KSharedConfig::openConfig(QStringLiteral("plasma_calendar_holiday_regions"));
    const KConfigGroup regionsConfig = m_config->group("General");
    updateSettings(regionsConfig);

    m_configWatcher = KConfigWatcher::create(m_config);
    connect(m_configWatcher.get(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &config) {
        updateSettings(config);
        loadEventsForDateRange(m_lastStartDate, m_lastEndDate);
    });
}

HolidaysEventsPlugin::~HolidaysEventsPlugin()
{
    qDeleteAll(m_regions);
}

void HolidaysEventsPlugin::loadEventsForDateRange(const QDate &startDate, const QDate &endDate)
{
    if (!m_lastData.empty() && m_lastStartDate == startDate && m_lastEndDate == endDate) {
        Q_EMIT dataReady(m_lastData);
        return;
    }

    m_lastData.clear();
    QMultiHash<QDate, CalendarEvents::EventData> data;

    for (KHolidays::HolidayRegion *region : qAsConst(m_regions)) {
        const KHolidays::Holiday::List holidays = region->rawHolidays(startDate, endDate);

        for (const KHolidays::Holiday &holiday : holidays) {
            CalendarEvents::EventData eventData;
            eventData.setStartDateTime(holiday.observedStartDate().startOfDay());
            eventData.setEndDateTime(holiday.observedEndDate().endOfDay());
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

    Q_EMIT dataReady(data);
}

void HolidaysEventsPlugin::updateSettings(const KConfigGroup &regionsConfig)
{
    QStringList regionCodes = regionsConfig.readEntry("selectedRegions", QStringList());
    regionCodes.removeDuplicates();

    // If the config does not have any region stored
    // add the default one
    if (regionCodes.empty()) {
        regionCodes << KHolidays::HolidayRegion::defaultRegionCode();
    }

    qDeleteAll(m_regions);
    m_regions.clear();

    m_regions.reserve(regionCodes.size());
    for (const QString &region : std::as_const(regionCodes)) {
        m_regions << new KHolidays::HolidayRegion(region);
    }

    if (!m_lastData.empty()) {
        for (const CalendarEvents::EventData &data : std::as_const(m_lastData)) {
            Q_EMIT eventRemoved(data.uid());
        }
        m_lastData.clear();
    }
}