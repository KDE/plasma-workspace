/*
 * Copyright 2007 Robert Knight <robertknight@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include <QDebug>

#include "tasksengine.h"
#include "virtualdesktopssource.h"
// own
#include "taskservice.h"
#include "taskwindowservice.h"

TasksEngine::TasksEngine(QObject *parent, const QVariantList &args) :
    Plasma::DataEngine(parent, args),
    m_groupManager(new TaskManager::GroupManager(this)),
    m_tasksModel(new TaskManager::TasksModel(m_groupManager, this))
{
    Q_UNUSED(args);
    //TODO HACK Remove
    //TasksModel does not initialize itself, So we need to set grouping strategy.
    m_groupManager->setGroupingStrategy(static_cast<TaskManager::GroupManager::TaskGroupingStrategy>(0));
    setModel("tasks", m_tasksModel);
}

TasksEngine::~TasksEngine()
{
}

Plasma::Service *TasksEngine::serviceForSource(const QString &name)
{
    Plasma::Service *service;
    if (name.isEmpty())
    {
	service = new TaskWindowService();
    } else if (name == "tasks") {
	service = new TaskService(m_tasksModel, m_groupManager);
    } else {
        service = Plasma::DataEngine::serviceForSource(name);
    }
    service->setParent(this);
    return service;
}

bool TasksEngine::sourceRequestEvent(const QString &source)
{
    if (source == "virtualDesktops") {
        addSource(new VirtualDesktopsSource);
        return true;
    }
    return false;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(tasks, TasksEngine, "plasma-dataengine-tasks.json")

#include "tasksengine.moc"
