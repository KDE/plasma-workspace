/*
    SPDX-FileCopyrightText: 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "jobcontrol.h"
#include "jobaction.h"
#include "kuiserverengine.h"

using namespace NotificationManager;

JobControl::JobControl(QObject *parent, Job *job)
    : Plasma::Service(parent)
    , m_job(job)
{
    setName(QStringLiteral("applicationjobs"));
    setDestination(KuiserverEngine::sourceName(job));
}

Plasma::ServiceJob *JobControl::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    return new JobAction(m_job, operation, parameters, this);
}
