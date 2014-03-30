/*
 * Copyright 2013 Heena Mahour<heena393@gmail.com>
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

#include "taskwindowjob.h"
#include <QDBusInterface>
#include <QtDBus/QDBusPendingCallWatcher>

TaskWindowJob::TaskWindowJob(const QString &source, const QString &operation , QMap<QString, QVariant> &parameters, QObject *parent) :
    ServiceJob(source, operation, parameters, parent),
    m_id(source)
{
}

TaskWindowJob::~TaskWindowJob()
{
}

void TaskWindowJob::start()
{
    const QString operation = operationName();
    if (operation == "cascade") {
        QDBusInterface  *kwinInterface = new QDBusInterface("org.kde.kwin", "/KWin", "org.kde.KWin");
        QDBusPendingCall pcall = kwinInterface->asyncCall("cascadeDesktop");
       // kDebug() << " connected to kwin interface! ";
        setResult(true);
        return;
    } else if (operation == "unclutter") {
        QDBusInterface  *kwinInterface = new QDBusInterface("org.kde.kwin", "/KWin", "org.kde.KWin");
        QDBusPendingCall pcall = kwinInterface->asyncCall("unclutterDesktop");
      //  kDebug() << "connected to kwin interface! ";
        setResult(true);
        return;
    } 
}

#include "taskwindowjob.moc"
