/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "notificationmanagerplugin.h"

#include "notifications.h"
#include "job.h"
#include "server.h"
#include "serverinfo.h"
#include "settings.h"
#include "watchednotificationsmodel.h"

#include <QQmlEngine>

using namespace NotificationManager;

void NotificationManagerPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.notificationmanager"));

    qmlRegisterType<Notifications>(uri, 1, 0, "Notifications");
    qmlRegisterUncreatableType<Job>(uri, 1, 0, "Job", QStringLiteral("Can only access Job via JobDetailsRole of JobsModel"));
    qmlRegisterType<Settings>(uri, 1, 0, "Settings");
    qmlRegisterSingletonType<Server>(uri, 1, 0, "Server", [](QQmlEngine *, QJSEngine *) -> QObject* {
        QQmlEngine::setObjectOwnership(&Server::self(), QQmlEngine::CppOwnership);
        return &Server::self();
    });
    qmlRegisterUncreatableType<ServerInfo>(uri, 1, 0, "ServerInfo", QStringLiteral("Can only access ServerInfo via Server"));

    // WARNING: this is unstable API and does not provide any API or ABI gurantee for future Plasma releases and can be removed without any further notice
    qmlRegisterType<WatchedNotificationsModel>(uri, 1, 1, "WatchedNotificationsModel");
}
