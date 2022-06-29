/*
    SPDX-FileCopyrightText: 2015-2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "taskmanagerplugin.h"

#include "abstracttasksmodel.h"
#include "activityinfo.h"
#include "tasksmodel.h"
#include "virtualdesktopinfo.h"

#ifdef WITH_KPIPEWIRE
#include <pipewiresourceitem.h>
#endif
#include "screencasting.h"
#include "screencastingrequest.h"

namespace TaskManager
{
void TaskManagerPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArrayLiteral("org.kde.taskmanager"));

    // Expose the AbstractTasksModel::AdditionalRoles enum to Qt Quick
    // for use with the TasksModel::data invokable. TasksModel inherits
    // the data roles from its source model, despite not inheriting from
    // AbstractTasksModel to avoid multiple inheritance from QObject-
    // derived classes.
    qmlRegisterUncreatableType<AbstractTasksModel>(uri, 0, 1, "AbstractTasksModel", "");

    qmlRegisterType<TasksModel>(uri, 0, 1, "TasksModel");
    qmlRegisterType<ActivityInfo>(uri, 0, 1, "ActivityInfo");
    qmlRegisterType<VirtualDesktopInfo>(uri, 0, 1, "VirtualDesktopInfo");
#ifdef WITH_KPIPEWIRE
    qmlRegisterType<PipeWireSourceItem>(uri, 0, 1, "PipeWireSourceItem");
#endif
    qmlRegisterType<ScreencastingRequest>(uri, 0, 1, "ScreencastingRequest");
    qmlRegisterUncreatableType<Screencasting>(uri, 0, 1, "Screencasting", "Use ScreencastingItem");
}

}
