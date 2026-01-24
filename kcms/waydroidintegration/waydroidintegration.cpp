/**
 * SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KPluginFactory>

#include <KConfigGroup>
#include <KQuickManagedConfigModule>
#include <KSharedConfig>

class KCMWaydroidIntegration : public KQuickManagedConfigModule
{
    Q_OBJECT

public:
    KCMWaydroidIntegration(QObject *parent, const KPluginMetaData &data)
        : KQuickManagedConfigModule(parent, data)
    {
        setButtons({});
    }

private:
    KSharedConfig::Ptr m_config;
};

K_PLUGIN_CLASS_WITH_JSON(KCMWaydroidIntegration, "kcm_waydroidintegration.json")

#include "waydroidintegration.moc"
