/*
  Copyright (c) 2015 Martin Klapetek <mklapetek@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "holidayeventshelperplugin.h"

#include <qqml.h>
#include <QDebug>

#include <KSharedConfig>
#include <KConfigGroup>

class QmlConfigHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList selectedRegions READ selectedRegions NOTIFY selectedRegionsChanged)

public:
    explicit QmlConfigHelper(QObject *parent = nullptr) : QObject(parent)
    {
        KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("plasma_calendar_holiday_regions"));
        m_configGroup = config->group("General");
        m_regions = m_configGroup.readEntry("selectedRegions", QStringList());
    }

    QStringList selectedRegions() const
    {
        return m_regions;
    }

    Q_INVOKABLE void saveConfig()
    {
        m_configGroup.writeEntry("selectedRegions", m_regions);
        m_configGroup.sync();
    }

    Q_INVOKABLE void addRegion(const QString &region)
    {
        if (!m_regions.contains(region)) {
            m_regions.append(region);
            Q_EMIT selectedRegionsChanged();
        }
    }

    Q_INVOKABLE void removeRegion(const QString &region)
    {
        if (m_regions.removeOne(region)) {
            Q_EMIT selectedRegionsChanged();
        }
    }

Q_SIGNALS:
    void selectedRegionsChanged();

private:
    QStringList m_regions;
    KConfigGroup m_configGroup;
};

void HolidayEventsHelperPlugin::registerTypes(const char* uri)
{
    qmlRegisterType<QmlConfigHelper>(uri, 1, 0, "QmlConfigHelper");
}

#include "holidayeventshelperplugin.moc"
