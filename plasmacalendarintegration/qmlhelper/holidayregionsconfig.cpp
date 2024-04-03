/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "holidayregionsconfig.h"

#include <KSharedConfig>

HolidayRegionsConfig::HolidayRegionsConfig(QObject *parent)
    : QObject(parent)
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("plasma_calendar_holiday_regions"));
    m_configGroup = config->group(QStringLiteral("General"));
    m_regions = m_configGroup.readEntry("selectedRegions", QStringList());
}

QStringList HolidayRegionsConfig::selectedRegions() const
{
    return m_regions;
}

void HolidayRegionsConfig::saveConfig()
{
    m_configGroup.writeEntry("selectedRegions", m_regions, KConfig::Notify);
    m_configGroup.sync();
}

void HolidayRegionsConfig::addRegion(const QString &region)
{
    if (!m_regions.contains(region)) {
        m_regions.append(region);
        Q_EMIT selectedRegionsChanged();
    }
}

void HolidayRegionsConfig::removeRegion(const QString &region)
{
    if (m_regions.removeOne(region)) {
        Q_EMIT selectedRegionsChanged();
    }
}

#include "moc_holidayregionsconfig.cpp"
