/*
    SPDX-FileCopyrightText: 2009 Jan Gerrit Marker <jangerrit@weiler-marker.com>
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QReadWriteLock>
#include <QTimer>

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
    /** The trigger word */
    QString m_triggerWord;

    /** How to sort */
    Sort m_sorting;

    /** process lister */
    KSysGuard::Processes *m_processes;

    /** lock for initializing m_processes */
    QReadWriteLock m_prepLock;

    /** timer for retrying the cleanup due to lock contention */
    QTimer m_delayedCleanupTimer;

    KRunner::Actions m_actionList;

    /** Reuse value */
    bool m_hasTrigger;

    // If the process list needs to be refreshed when matching
    bool m_needsRefresh;
};
