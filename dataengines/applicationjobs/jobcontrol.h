/*
    SPDX-FileCopyrightText: 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <plasma/service.h>

#include <QPointer>

#include "job.h"

class JobControl : public Plasma::Service
{
    Q_OBJECT

public:
    JobControl(QObject *parent, NotificationManager::Job *job);

protected:
    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

private:
    QPointer<NotificationManager::Job> m_job;
};
