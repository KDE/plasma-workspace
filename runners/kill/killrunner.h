/*
    SPDX-FileCopyrightText: 2009 Jan Gerrit Marker <jangerrit@weiler-marker.com>
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <KRunner/AbstractRunner>
#include <KRunner/Action>

#include "config_keys.h"

namespace KSysGuard
{
class Processes;
class Process;
}

class KillRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    KillRunner(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;
    void reloadConfiguration() override;

private:
    const KRunner::Actions m_actionList;
    QString m_triggerWord;
    bool m_hasTrigger = false;

    // process lister
    KSysGuard::Processes *const m_processes;
    Sort m_sorting;

    // If the process list needs to be refreshed when matching. This is only done once the trigger word (if set) is used
    bool m_needsRefresh;
};
