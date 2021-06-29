/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "holidayeventshelperplugin.h"

#include <QDebug>
#include <qqml.h>

#include <KConfigGroup>
#include <KSharedConfig>

class QmlConfigHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList selectedRegions READ selectedRegions NOTIFY selectedRegionsChanged)

public:
    explicit QmlConfigHelper(QObject *parent = nullptr)
        : QObject(parent)
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

void HolidayEventsHelperPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<QmlConfigHelper>(uri, 1, 0, "QmlConfigHelper");
}

#include "holidayeventshelperplugin.moc"
