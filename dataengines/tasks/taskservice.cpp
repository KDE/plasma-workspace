/*
 * Copyright 2008 Alain Boyer <alainboyer@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <tasksmodel.h>
#include <groupmanager.h>

#include "taskservice.h"

// own
#include "taskjob.h"

TaskService::TaskService(TaskManager::TasksModel *model, TaskManager::GroupManager *groupManager) :
    Plasma::Service(),
    m_model(model),
    m_groupManager(groupManager)
{
    setName("tasks");
}

TaskService::~TaskService()
{
}

Plasma::ServiceJob *TaskService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    return new TaskJob(m_model, m_groupManager, operation, parameters, this);
}

#include "taskservice.moc"
