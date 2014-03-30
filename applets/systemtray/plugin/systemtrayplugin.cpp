/*
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "systemtrayplugin.h"

#include "host.h"
#include "task.h"
#include "protocols/plasmoid/plasmoidtask.h"
#include "protocols/dbussystemtray/dbussystemtraytask.h"

#include <QtQml>

#include "debug.h"

Q_LOGGING_CATEGORY(SYSTEMTRAY, "systemtray")

namespace SystemTray {

void SystemTrayPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QStringLiteral("org.kde.private.systemtray"));
    QLoggingCategory::setFilterRules(QStringLiteral("systemtray.debug = true"));

    qmlRegisterType<SystemTray::Host>(uri, 2, 0,"Host");
    qmlRegisterUncreatableType<SystemTray::Task>(uri, 2, 0, "Task", "You cannot create Task objects.");
    qmlRegisterUncreatableType<SystemTray::DBusSystemTrayTask>(uri, 2, 0, "DBusSystemTrayTask", "You cannot create Task objects.");
    qmlRegisterUncreatableType<SystemTray::PlasmoidTask>(uri, 2, 0, "PlasmoidTask", "You cannot create Task objects.");
    qmlRegisterUncreatableType<SystemTray::Task>(uri, 2, 0, "Task", "You cannot create Task objects.");

    qCDebug(SYSTEMTRAY) << "Categorized debug";
}

} // namespace

#include "systemtrayplugin.moc"