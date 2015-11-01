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
    TaskManager::AbstractGroupableItem* item = m_groupManager->rootGroup()->getMemberById(parameters().value(QStringLiteral("Id")).toInt());

    if (!item) {
        return;
    }

    TaskManager::TaskItem* taskItem = static_cast<TaskManager::TaskItem*>(item);

    // only a subset of task operations are exported
    const QString operation = operationName();
    if (operation == QLatin1String("setMaximized")) {
        taskItem->task()->setMaximized(parameters().value(QStringLiteral("maximized")).toBool());
        setResult(true);
        return;
    } else if (operation == QLatin1String("setMinimized")) {
        taskItem->task()->setIconified(parameters().value(QStringLiteral("minimized")).toBool());
        setResult(true);
        return;
    } else if (operation == QLatin1String("setShaded")) {
        taskItem->task()->setShaded(parameters().value(QStringLiteral("shaded")).toBool());
        setResult(true);
        return;
    } else if (operation == QLatin1String("setFullScreen")) {
        taskItem->task()->setFullScreen(parameters().value(QStringLiteral("fullScreen")).toBool());
        setResult(true);
        return;
    } else if (operation == QLatin1String("setAlwaysOnTop")) {
        taskItem->task()->setAlwaysOnTop(parameters().value(QStringLiteral("alwaysOnTop")).toBool());
        setResult(true);
        return;
    } else if (operation == QLatin1String("setKeptBelowOthers")) {
        taskItem->task()->setKeptBelowOthers(parameters().value(QStringLiteral("keptBelowOthers")).toBool());
        setResult(true);
        return;
    } else if (operation == QLatin1String("toggleMaximized")) {
        taskItem->task()->toggleMaximized();
        setResult(true);
        return;
    } else if (operation == QLatin1String("toggleMinimized")) {
        taskItem->task()->toggleIconified();
        setResult(true);
        return;
    } else if (operation == QLatin1String("toggleShaded")) {
        taskItem->task()->toggleShaded();
        setResult(true);
        return;
    } else if (operation == QLatin1String("toggleFullScreen")) {
        taskItem->task()->toggleFullScreen();
        setResult(true);
        return;
    } else if (operation == QLatin1String("toggleAlwaysOnTop")) {
        taskItem->task()->toggleAlwaysOnTop();
        setResult(true);
        return;
    } else if (operation == QLatin1String("toggleKeptBelowOthers")) {
        taskItem->task()->toggleKeptBelowOthers();
        setResult(true);
        return;
    } else if (operation == QLatin1String("restore")) {
        taskItem->task()->restore();
        setResult(true);
        return;
    } else if (operation == QLatin1String("resize")) {
        taskItem->task()->resize();
        setResult(true);
        return;
    } else if (operation == QLatin1String("move")) {
        taskItem->task()->move();
        setResult(true);
        return;
    } else if (operation == QLatin1String("raise")) {
        taskItem->task()->raise();
        setResult(true);
        return;
    } else if (operation == QLatin1String("lower")) {
        taskItem->task()->lower();
        setResult(true);
        return;
    } else if (operation == QLatin1String("activate")) {
        taskItem->task()->activate();
        setResult(true);
        return;
    } else if (operation == QLatin1String("activateRaiseOrIconify")) {
        taskItem->task()->activateRaiseOrIconify();
        setResult(true);
        return;
    } else if (operation == QLatin1String("close")) {
        taskItem->task()->close();
        setResult(true);
        return;
    } else if (operation == QLatin1String("toDesktop")) {
        taskItem->task()->toDesktop(parameters().value(QStringLiteral("desktop")).toInt());
        setResult(true);
        return;
    } else if (operation == QLatin1String("toCurrentDesktop")) {
        taskItem->task()->toCurrentDesktop();
        setResult(true);
        return;
    } else if (operation == QLatin1String("publishIconGeometry")) {
        taskItem->task()->publishIconGeometry(parameters().value(QStringLiteral("geometry")).toRect());
        setResult(true);
        return;
    }

    setResult(false);
}

