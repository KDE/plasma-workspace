/*
    SPDX-FileCopyrightText: 2010 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "activityservice.h"
#include "activityjob.h"

ActivityService::ActivityService(KActivities::Controller *controller, const QString &source)
    : m_activityController(controller)
    , m_id(source)
{
    setName(QStringLiteral("activities"));
}

ServiceJob *ActivityService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    return new ActivityJob(m_activityController, m_id, operation, parameters, this);
}
