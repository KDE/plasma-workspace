/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
