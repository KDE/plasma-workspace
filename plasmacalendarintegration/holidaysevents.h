/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <CalendarEvents/CalendarEventsPlugin>
#include <QObject>

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
    QList<KHolidays::HolidayRegion *> m_regions;
    QMultiHash<QDate, CalendarEvents::EventData> m_lastData;
    KSharedConfig::Ptr m_config;
};
