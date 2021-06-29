/*
    SPDX-FileCopyrightText: 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef NOTIFICATIONSERVICE_H
#define NOTIFICATIONSERVICE_H

#include <Plasma/Service>

class NotificationsEngine;

class NotificationService : public Plasma::Service
{
    Q_OBJECT

public:
    NotificationService(NotificationsEngine *parent, const QString &source);

protected:
    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

private:
    NotificationsEngine *m_notificationEngine;
};

#endif // NOTIFICATIONSERVICE_H
