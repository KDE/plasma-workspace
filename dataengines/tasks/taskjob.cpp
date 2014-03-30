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

#include "taskjob.h"

TaskJob::TaskJob(const TaskManager::TasksModel *model, const TaskManager::GroupManager *groupManager, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent) :
    ServiceJob(model->objectName(), operation, parameters, parent),
    m_model(model),
    m_groupManager(groupManager)
{
}

TaskJob::~TaskJob()
{
}

void TaskJob::start()
{
    TaskManager::AbstractGroupableItem* item = m_groupManager->rootGroup()->getMemberById(parameters().value("Id").toInt());

    if (!item) {
        return;
    }

    TaskManager::TaskItem* taskItem = static_cast<TaskManager::TaskItem*>(item);

    // only a subset of task operations are exported
    const QString operation = operationName();
    if (operation == "setMaximized") {
        taskItem->task()->setMaximized(parameters().value("maximized").toBool());
        setResult(true);
        return;
    } else if (operation == "setMinimized") {
        taskItem->task()->setIconified(parameters().value("minimized").toBool());
        setResult(true);
        return;
    } else if (operation == "setShaded") {
        taskItem->task()->setShaded(parameters().value("shaded").toBool());
        setResult(true);
        return;
    } else if (operation == "setFullScreen") {
        taskItem->task()->setFullScreen(parameters().value("fullScreen").toBool());
        setResult(true);
        return;
    } else if (operation == "setAlwaysOnTop") {
        taskItem->task()->setAlwaysOnTop(parameters().value("alwaysOnTop").toBool());
        setResult(true);
        return;
    } else if (operation == "setKeptBelowOthers") {
        taskItem->task()->setKeptBelowOthers(parameters().value("keptBelowOthers").toBool());
        setResult(true);
        return;
    } else if (operation == "toggleMaximized") {
        taskItem->task()->toggleMaximized();
        setResult(true);
        return;
    } else if (operation == "toggleMinimized") {
        taskItem->task()->toggleIconified();
        setResult(true);
        return;
    } else if (operation == "toggleShaded") {
        taskItem->task()->toggleShaded();
        setResult(true);
        return;
    } else if (operation == "toggleFullScreen") {
        taskItem->task()->toggleFullScreen();
        setResult(true);
        return;
    } else if (operation == "toggleAlwaysOnTop") {
        taskItem->task()->toggleAlwaysOnTop();
        setResult(true);
        return;
    } else if (operation == "toggleKeptBelowOthers") {
        taskItem->task()->toggleKeptBelowOthers();
        setResult(true);
        return;
    } else if (operation == "restore") {
        taskItem->task()->restore();
        setResult(true);
        return;
    } else if (operation == "resize") {
        taskItem->task()->resize();
        setResult(true);
        return;
    } else if (operation == "move") {
        taskItem->task()->move();
        setResult(true);
        return;
    } else if (operation == "raise") {
        taskItem->task()->raise();
        setResult(true);
        return;
    } else if (operation == "lower") {
        taskItem->task()->lower();
        setResult(true);
        return;
    } else if (operation == "activate") {
        taskItem->task()->activate();
        setResult(true);
        return;
    } else if (operation == "activateRaiseOrIconify") {
        taskItem->task()->activateRaiseOrIconify();
        setResult(true);
        return;
    } else if (operation == "close") {
        taskItem->task()->close();
        setResult(true);
        return;
    } else if (operation == "toDesktop") {
        taskItem->task()->toDesktop(parameters().value("desktop").toInt());
        setResult(true);
        return;
    } else if (operation == "toCurrentDesktop") {
        taskItem->task()->toCurrentDesktop();
        setResult(true);
        return;
    } else if (operation == "publishIconGeometry") {
        taskItem->task()->publishIconGeometry(parameters().value("geometry").toRect());
        setResult(true);
        return;
    }

    setResult(false);
}

#include "taskjob.moc"
