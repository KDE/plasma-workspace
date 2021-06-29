/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "notificationmanagerplugin.h"

#include "job.h"
#include "notifications.h"
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
    qmlRegisterSingletonType<Server>(uri, 1, 0, "Server", [](QQmlEngine *, QJSEngine *) -> QObject * {
        QQmlEngine::setObjectOwnership(&Server::self(), QQmlEngine::CppOwnership);
        return &Server::self();
    });
    qmlRegisterUncreatableType<ServerInfo>(uri, 1, 0, "ServerInfo", QStringLiteral("Can only access ServerInfo via Server"));

    // WARNING: this is unstable API and does not provide any API or ABI gurantee for future Plasma releases and can be removed without any further notice
    qmlRegisterType<WatchedNotificationsModel>(uri, 1, 1, "WatchedNotificationsModel");
}
