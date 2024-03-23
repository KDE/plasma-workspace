/*
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "globalconfig.h"

GlobalConfig::GlobalConfig(QObject *parent)
    : QObject(parent)
    , m_configWatcher(KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("plasmaparc"), KSharedConfig::NoGlobals)))
{
    connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this, &GlobalConfig::onConfigChanged);
    onConfigChanged();
}

GlobalConfig::~GlobalConfig()
{
}

void GlobalConfig::onConfigChanged()
{
    KConfigGroup group = m_configWatcher->config()->group(QStringLiteral("General"));
    int step = group.readEntry(QStringLiteral("VolumeStep"), 5);
    if (step != m_volumeStep) {
        m_volumeStep = step;
        Q_EMIT volumeStepChanged();
    }

    QString preferredPlayer = group.readEntry(QStringLiteral("MultiplexerPreferredPlayer"), QString());
    if (preferredPlayer != m_preferredPlayer) {
        m_preferredPlayer = std::move(preferredPlayer);
        Q_EMIT preferredPlayerChanged();
    }
}

int GlobalConfig::volumeStep() const
{
    return m_volumeStep;
}

QString GlobalConfig::preferredPlayer() const
{
    return m_preferredPlayer;
}
