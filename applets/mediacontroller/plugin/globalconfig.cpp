/*
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "globalconfig.h"

GlobalConfig::GlobalConfig(QObject *parent)
    : QObject(parent)
    , m_configWatcher(KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("plasmaparc"))))
{
    connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this, &GlobalConfig::configChanged);
    configChanged();
}

GlobalConfig::~GlobalConfig()
{
}

void GlobalConfig::configChanged()
{
    int step = m_configWatcher->config()->group(QStringLiteral("General")).readEntry(QStringLiteral("VolumeStep"), 5);
    if (step != m_volumeStep) {
        m_volumeStep = step;
        Q_EMIT volumeStepChanged();
    }
}

int GlobalConfig::volumeStep()
{
    return m_volumeStep;
}
