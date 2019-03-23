/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QObject>
#include <QDBusContext>
#include <QDBusUnixFileDescriptor>
#include <QVector>

//#include "notificationmanager_export.h"

namespace NotificationManager
{

class Notification;

/**
 * @short Registers an inhibition server on the DBus
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class InhibitionServer : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    ~InhibitionServer() override;

    static InhibitionServer &self();

    // DBus
    QDBusUnixFileDescriptor Inhibit(const QString &app_name, const QString &reason, const QVariantMap &hints);

Q_SIGNALS:
    void inhibitionAdded();
    void inhibitionRemoved();

private:
    explicit InhibitionServer(QObject *parent = nullptr);
    Q_DISABLE_COPY(InhibitionServer)
    // FIXME we also need to disable move and other stuff?

    // TODO list of fds
};

} // namespace NotificationManager
