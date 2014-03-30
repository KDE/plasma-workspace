/*
 *   Copyright 2010 Chani Armitage <chani@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ACTIVITY_SERVICE_H
#define ACTIVITY_SERVICE_H

#include "activityengine.h"

#include <Plasma/Service>
#include <Plasma/ServiceJob>

using namespace Plasma;

namespace KActivities
{
    class Controller;
} // namespace KActivities


class ActivityService : public Plasma::Service
{
    Q_OBJECT

public:
    ActivityService(KActivities::Controller *controller, const QString &source);
    ServiceJob *createJob(const QString &operation,
                          QMap<QString, QVariant> &parameters);

private:
    KActivities::Controller *m_activityController;
    QString m_id;
};

#endif // SEARCHLAUNCH_SERVICE_H
