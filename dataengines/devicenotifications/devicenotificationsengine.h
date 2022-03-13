/*
    SPDX-FileCopyrightText: 2010 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 Lukáš Tinkl <ltinkl@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Plasma/DataEngine>

#include "ksolidnotify.h"

/**
 *  Engine which provides data sources for device notifications.
 *  Each notification is represented by one source.
 */
class DeviceNotificationsEngine : public Plasma::DataEngine
{
    Q_OBJECT
public:
    DeviceNotificationsEngine(QObject *parent, const QVariantList &args);
    ~DeviceNotificationsEngine() override;

private Q_SLOTS:
    void notify(Solid::ErrorType solidError, const QString &error, const QString &errorDetails, const QString &udi);
    void clearNotification(const QString &udi);

private:
    KSolidNotify *const m_solidNotify;
};
