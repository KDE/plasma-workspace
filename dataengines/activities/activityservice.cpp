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

#include "activityservice.h"
#include "activityjob.h"

ActivityService::ActivityService(KActivities::Controller *controller, const QString &source)
    : m_activityController(controller),
      m_id(source)
{
    setName("activities");
}

ServiceJob *ActivityService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    return new ActivityJob(m_activityController, m_id, operation, parameters, this);
}

#include "activityservice.moc"
