/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

#include "notificationmanager_export.h"

#include <QObject>
#include <QScopedPointer>
#include <QString>

namespace NotificationManager
{
/**
 * @short Information about the notification server
 *
 * Provides information such as vendor, name, version of the notification server.
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NOTIFICATIONMANAGER_EXPORT ServerInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

    Q_PROPERTY(QString vendor READ vendor NOTIFY vendorChanged)

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)

    Q_PROPERTY(QString version READ version NOTIFY versionChanged)

    Q_PROPERTY(QString specVersion READ specVersion NOTIFY specVersionChanged)

public:
    explicit ServerInfo(QObject *parent = nullptr);
    ~ServerInfo() override;

    enum class Status {
        Unknown = -1,
        NotRunning,
        Running,
    };
    Q_ENUM(Status)

    Status status() const;
    QString vendor() const;
    QString name() const;
    QString version() const;
    QString specVersion() const;

Q_SIGNALS:
    void statusChanged(Status status);
    void vendorChanged(const QString &vendor);
    void nameChanged(const QString &name);
    void versionChanged(const QString &version);
    void specVersionChanged(const QString &specVersion);

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace NotificationManager
