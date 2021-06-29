/*
    SPDX-FileCopyrightText: 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "kuiserverengine.h"

#include <QPointer>

#include <plasma/servicejob.h>

#include "job.h"

namespace NotificationManager
{
class Job;
}

class JobAction : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    JobAction(NotificationManager::Job *job, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent = nullptr)
        : ServiceJob(KuiserverEngine::sourceName(job), operation, parameters, parent)
        , m_job(job)
    {
    }

    void start() override;

private:
    QPointer<NotificationManager::Job> m_job;
};
