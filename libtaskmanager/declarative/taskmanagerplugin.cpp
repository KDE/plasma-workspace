/*
    SPDX-FileCopyrightText: 2015-2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "taskmanagerplugin.h"

#include "abstracttasksmodel.h"
#include "activityinfo.h"
#include "tasksmodel.h"
#include "virtualdesktopinfo.h"

#ifdef WITH_PIPEWIRE
#include "pipewirerecord.h"
#include "pipewiresourceitem.h"
#include "screencasting.h"
#include "screencastingrequest.h"
#endif

namespace TaskManager
{
void TaskManagerPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.taskmanager"));

    // Expose the AbstractTasksModel::AdditionalRoles enum to Qt Quick
    // for use with the TasksModel::data invokable. TasksModel inherits
    // the data roles from its source model, despite not inheriting from
    // AbstractTasksModel to avoid multiple inheritance from QObject-
    // derived classes.
    qmlRegisterUncreatableType<AbstractTasksModel>(uri, 0, 1, "AbstractTasksModel", "");

    qmlRegisterType<TasksModel>(uri, 0, 1, "TasksModel");
    qmlRegisterType<ActivityInfo>(uri, 0, 1, "ActivityInfo");
    qmlRegisterType<VirtualDesktopInfo>(uri, 0, 1, "VirtualDesktopInfo");
#ifdef WITH_PIPEWIRE
    qmlRegisterType<PipeWireSourceItem>(uri, 0, 1, "PipeWireSourceItem");
    qmlRegisterType<PipeWireRecord>(uri, 0, 1, "PipeWireRecord");
    qmlRegisterType<ScreencastingRequest>(uri, 0, 1, "ScreencastingRequest");
    qmlRegisterUncreatableType<Screencasting>(uri, 0, 1, "Screencasting", "Use ScreencastingItem");
#endif
}

}
