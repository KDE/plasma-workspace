/*
    SPDX-FileCopyrightText: 2010 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "activityengine.h"

#include <Plasma5Support/Service>
#include <Plasma5Support/ServiceJob>

using namespace Plasma5Support;

namespace KActivities
{
class Controller;
} // namespace KActivities

class ActivityService : public Plasma5Support::Service
{
    Q_OBJECT

public:
    ActivityService(KActivities::Controller *controller, const QString &source);
    ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

private:
    KActivities::Controller *m_activityController;
    QString m_id;
};
