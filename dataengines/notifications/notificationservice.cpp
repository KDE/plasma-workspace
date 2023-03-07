/*
    SPDX-FileCopyrightText: 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "notificationservice.h"
#include "notificationaction.h"
#include "notificationsengine.h"

NotificationService::NotificationService(NotificationsEngine *parent, const QString &source)
    : Plasma5Support::Service(parent)
    , m_notificationEngine(parent)
{
    setName(QStringLiteral("notifications"));
    setDestination(source);
}

Plasma5Support::ServiceJob *NotificationService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    return new NotificationAction(m_notificationEngine, destination(), operation, parameters, this);
}
