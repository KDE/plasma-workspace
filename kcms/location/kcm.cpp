/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"
#include "settings.h"

K_PLUGIN_FACTORY_WITH_JSON(LocationKcmFactory, "kcm_location.json", registerPlugin<LocationKcm>(); registerPlugin<LocationData>();)

LocationKcm::LocationKcm(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new LocationData(this))
{
}

LocationSettings *LocationKcm::settings() const
{
    return m_data->settings();
}

#include "kcm.moc"
#include "moc_kcm.cpp"
