/*
 * Copyright (C) 2010 Jacopo De Simoi <wilderkde@gmail.com>
 * Copyright (C) 2014 by Lukáš Tinkl <ltinkl@redhat.com>
 *
 * This program is free software you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "devicenotificationsengine.h"

#include <Plasma/DataContainer>

DeviceNotificationsEngine::DeviceNotificationsEngine( QObject* parent, const QVariantList& args )
    : Plasma::DataEngine( parent, args ),
      m_solidNotify(new KSolidNotify(this))
{
    connect(m_solidNotify, &KSolidNotify::notify, this, &DeviceNotificationsEngine::notify);
    connect(m_solidNotify, &KSolidNotify::clearNotification, this, &DeviceNotificationsEngine::clearNotification);
}

DeviceNotificationsEngine::~DeviceNotificationsEngine()
{
}

void DeviceNotificationsEngine::notify(Solid::ErrorType solidError, const QString& error, const QString& errorDetails, const QString &udi)
{
    const QString source = QStringLiteral("%1 notification").arg(udi);

    Plasma::DataEngine::Data notificationData;
    notificationData.insert(QStringLiteral("solidError"), solidError);
    notificationData.insert(QStringLiteral("error"), error);
    notificationData.insert(QStringLiteral("errorDetails"), errorDetails);
    notificationData.insert(QStringLiteral("udi"), udi);

    setData(source, notificationData );
}

void DeviceNotificationsEngine::clearNotification(const QString &udi)
{
    removeSource(QStringLiteral("%1 notification").arg(udi));
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(devicenotifications, DeviceNotificationsEngine, "plasma-dataengine-devicenotifications.json")

#include "devicenotificationsengine.moc"
