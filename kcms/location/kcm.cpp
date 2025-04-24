/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"
#include "settings.h"

K_PLUGIN_FACTORY_WITH_JSON(LocationKcmFactory, "kcm_location.json", registerPlugin<LocationKcm>();)

LocationKcm::LocationKcm(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_settings(new GeoClueSettings(this))
{
    connect(m_settings, &GeoClueSettings::changed, this, &LocationKcm::settingsChanged);
}

void LocationKcm::load()
{
    m_settings->load();
    KQuickManagedConfigModule::load();
}

void LocationKcm::save()
{
    m_settings->save();
    KQuickManagedConfigModule::save();
}

void LocationKcm::defaults()
{
    m_settings->defaults();
    KQuickManagedConfigModule::defaults();
}

bool LocationKcm::isSaveNeeded() const
{
    return m_settings->isSaveNeeded();
}

bool LocationKcm::isDefaults() const
{
    return m_settings->isDefaults();
}

#include "kcm.moc"
#include "moc_kcm.cpp"
