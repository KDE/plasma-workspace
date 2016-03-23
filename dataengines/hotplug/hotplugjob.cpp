/*
 *   Copyright (C) 2011 Viranch Mehta <viranch.mehta@gmail.com>
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

#include "hotplugjob.h"
#include "hotplugengine.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <QDebug>

void HotplugJob::start()
{
    QString udi (m_dest);
    QString operation = operationName();
    
    if (operation == QLatin1String("invokeAction")) {
        QString action = parameters()[QStringLiteral("predicate")].toString();

        QStringList desktopFiles;
        desktopFiles << action;

        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kded5"),
                                                          QStringLiteral("/modules/soliduiserver"),
                                                          QStringLiteral("org.kde.SolidUiServer"),
                                                          QStringLiteral("showActionsDialog"));

        msg.setArguments(QList<QVariant>() << udi << desktopFiles);
        QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
    }

    emitResult();
}



