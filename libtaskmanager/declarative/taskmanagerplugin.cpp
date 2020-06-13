/********************************************************************
Copyright 2015-2016  Eike Hein <hein.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "taskmanagerplugin.h"

#include "abstracttasksmodel.h"
#include "tasksmodel.h"
#include "activityinfo.h"
#include "virtualdesktopinfo.h"

#ifdef WITH_PIPEWIRE
#include "pipewiresourceitem.h"
#include "screencastingrequest.h"
#include "screencasting.h"
#endif

#include <QQmlEngine>

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
    qmlRegisterType<ScreencastingRequest>(uri, 0, 1, "ScreencastingRequest");
    qmlRegisterUncreatableType<Screencasting>(uri, 0, 1, "Screencasting", "Use ScreencastingItem");
#endif
}

}
