/*
    SPDX-FileCopyrightText: 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "notificationsengine.h"

#include <Plasma/ServiceJob>

class NotificationAction : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    NotificationAction(NotificationsEngine *engine,
                       const QString &destination,
                       const QString &operation,
                       QMap<QString, QVariant> &parameters,
                       QObject *parent = nullptr)
        : ServiceJob(destination, operation, parameters, parent)
        , m_engine(engine)
    {
    }

    void start() override;

private:
    NotificationsEngine *const m_engine;
};
