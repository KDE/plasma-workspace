/* Copyright 2009  <Jan Gerrit Marker> <jangerrit@weiler-marker.com>
 * Copyright 2020  <Alexander Lohnau> <alexander.lohnau@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KILLRUNNER_H
#define KILLRUNNER_H

#include <QReadWriteLock>
#include <QTimer>

#include <KRunner/AbstractRunner>

#include "config_keys.h"
class QAction;

namespace KSysGuard
{
    class Processes;
    class Process;
}

class KillRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    KillRunner(QObject *parent, const QVariantList &args);
    ~KillRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;
    QList<QAction*> actionsForMatch(const Plasma::QueryMatch &match) override;
    void reloadConfiguration() override;

private Q_SLOTS:
    void prep();
    void cleanup();

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

    /** Reuse actions */
    QList<QAction *> m_actionList;

    /** Reuse value */
    bool m_hasTrigger;
};

#endif
