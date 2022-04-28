/*
    SPDX-FileCopyrightText: 2010 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 Lukáš Tinkl <ltinkl@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "devicenotificationsengine.h"

#include <Plasma/DataContainer>

DeviceNotificationsEngine::DeviceNotificationsEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
    , m_solidNotify(new KSolidNotify(this))
{
    connect(m_solidNotify, &KSolidNotify::notify, this, &DeviceNotificationsEngine::notify);
    connect(m_solidNotify, &KSolidNotify::clearNotification, this, &DeviceNotificationsEngine::clearNotification);
}

void DeviceNotificationsEngine::notify(Solid::ErrorType solidError, const QString &error, const QString &errorDetails, const QString &udi)
{
    const QString source = QStringLiteral("%1 notification").arg(udi);

    Plasma::DataEngine::Data notificationData;
    notificationData.insert(QStringLiteral("solidError"), solidError);
    notificationData.insert(QStringLiteral("error"), error);
    notificationData.insert(QStringLiteral("errorDetails"), errorDetails);
    notificationData.insert(QStringLiteral("udi"), udi);

    setData(source, notificationData);
}

void DeviceNotificationsEngine::clearNotification(const QString &udi)
{
    removeSource(QStringLiteral("%1 notification").arg(udi));
}

K_PLUGIN_CLASS_WITH_JSON(DeviceNotificationsEngine, "plasma-dataengine-devicenotifications.json")

#include "devicenotificationsengine.moc"
