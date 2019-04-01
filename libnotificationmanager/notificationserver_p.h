/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

class QDBusServiceWatcher;

struct Inhibition
{
    QString desktopEntry;
    QString reason;
    QVariantMap hints;
};

namespace NotificationManager
{

class NotificatonServer;

class Q_DECL_HIDDEN NotificationServerPrivate : public QObject, protected QDBusContext
{
    Q_OBJECT

    // DBus
    // Inhibitions
    Q_PROPERTY(bool Inhibited READ inhibited)

public:
    NotificationServerPrivate(QObject *parent);
    ~NotificationServerPrivate() override;

    // DBus
    uint Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int timeout);
    void CloseNotification(uint id);
    QStringList GetCapabilities() const;
    QString GetServerInformation(QString &vendor, QString &version, QString &specVersion) const;

    // Inhibitions
    uint Inhibit(const QString &desktop_entry,
                 const QString &reason,
                 const QVariantMap &hints);
    void UnInhibit(uint cookie);
    QList<Inhibition> ListInhibitors() const;
    bool inhibited() const; // property getter

Q_SIGNALS:
    // DBus
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);

    void inhibitedChanged();

public: // stuff used by public class
    bool m_valid = false;

private:
    void onServiceUnregistered(const QString &serviceName);

    uint m_highestNotificationId = 0;

    QDBusServiceWatcher *m_inhibitionWatcher = nullptr;
    uint m_highestInhibitionCookie = 0;
    QHash<uint /*cookie*/, Inhibition> m_inhibitions;
    QHash<uint /*cookie*/, QString> m_inhibitionServices;

};

} // namespace NotificationManager
