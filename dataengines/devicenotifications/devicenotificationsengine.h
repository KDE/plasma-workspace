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


#ifndef DEVICENOTIFICATIONSENGINE_H
#define DEVICENOTIFICATIONSENGINE_H

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
    DeviceNotificationsEngine( QObject* parent, const QVariantList& args );
    ~DeviceNotificationsEngine() override;

private slots:
     void notify(Solid::ErrorType solidError, const QString& error, const QString& errorDetails, const QString &udi);
    void clearNotification(const QString &udi);

private:
    KSolidNotify * m_solidNotify;
};

#endif
