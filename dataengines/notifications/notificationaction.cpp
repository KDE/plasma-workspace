/*
 *   Copyright © 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>
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

#include "notificationaction.h"
#include "notificationsengine.h"

#include <klocalizedstring.h>

#include <QDebug>

void NotificationAction::start()
{
    //qDebug() << "Trying to perform the action " << operationName() << " on " << destination();
    //qDebug() << "actionId: " << parameters()["actionId"].toString();
    //qDebug() << "params: " << parameters();

    if (!m_engine) {
        setErrorText(i18n("The notification dataEngine is not set."));
        setError(-1);
        emitResult();
        return;
    }

    const QStringList dest = destination().split(' ');

    uint id = 0;
    if (dest.count() >  1 && !dest[1].toInt()) {
        setErrorText(i18n("Invalid destination: %1", destination()));
        setError(-2);
        emitResult();
        return;
    } else if (dest.count() >  1) {
        id = dest[1].toUInt();
    }

    if (operationName() == QLatin1String("invokeAction")) {
        //qDebug() << "invoking action on " << id;
        emit m_engine->ActionInvoked(id, parameters()[QStringLiteral("actionId")].toString());
    } else if (operationName() == QLatin1String("userClosed")) {
        //userClosedNotification deletes the job, so we have to invoke it queued, in this case emitResult() can be called
        m_engine->metaObject()->invokeMethod(m_engine, "removeNotification", Qt::QueuedConnection, Q_ARG(uint, id), Q_ARG(uint, 2));
    } else if (operationName() == QLatin1String("expireNotification")) {
        //expireNotification deletes the job, so we have to invoke it queued, in this case emitResult() can be called
        m_engine->metaObject()->invokeMethod(m_engine, "removeNotification", Qt::QueuedConnection, Q_ARG(uint, id), Q_ARG(uint, 1));
    } else if (operationName() == QLatin1String("createNotification")) {
        int expireTimeout = parameters().value(QStringLiteral("expireTimeout")).toInt();
        bool isPersistent = parameters().value(QStringLiteral("isPersistent")).toBool();

        QVariantMap hints;
        if (parameters().value(QStringLiteral("skipGrouping")).toBool()) {
            hints.insert(QStringLiteral("x-kde-skipGrouping"), true);
        }

        int rv = m_engine->createNotification(parameters().value(QStringLiteral("appName")).toString(),
                                              parameters().value(QStringLiteral("appIcon")).toString(),
                                              parameters().value(QStringLiteral("summary")).toString(),
                                              parameters().value(QStringLiteral("body")).toString(),
                                              isPersistent ? 0 : expireTimeout,
                                              parameters().value(QStringLiteral("actions")).toStringList(),
                                              hints);
        setResult(rv);
    } else if (operationName() == QLatin1String("configureNotification")) {
        m_engine->configureNotification(parameters()[QStringLiteral("appRealName")].toString(),
                                        parameters()[QStringLiteral("eventId")].toString());
    }

    emitResult();
}



