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
#include <QStringList>

#include "notification.h"

class QDBusServiceWatcher;

struct Inhibition
{
    QString desktopEntry;
    QString applicationName;
    //QString applicationIconName;
    QString reason;
    QVariantMap hints;
};

namespace NotificationManager
{

class ServerInfo;

class Q_DECL_HIDDEN ServerPrivate : public QObject, protected QDBusContext
{
    Q_OBJECT

    // DBus
    // Inhibitions
    Q_PROPERTY(bool Inhibited READ inhibited)

public:
    ServerPrivate(QObject *parent);
    ~ServerPrivate() override;

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
    bool inhibited() const; // property getter

    void InvokeAction(uint id, const QString &actionKey);

Q_SIGNALS:
    // DBus
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);
    // non-standard
    // This is manually emitted as targeted signal in sendReplyText()
    void NotificationReplied(uint id, const QString &text);

    void validChanged();

    void inhibitedChanged();

    void externalInhibitedChanged();
    void externalInhibitionsChanged();

    void serviceOwnershipLost();

public: // stuff used by public class
    friend class ServerInfo;
    static QString notificationServiceName();
    static QString notificationServicePath();
    static QString notificationServiceInterface();

    bool init();
    uint add(const Notification &notification);
    void sendReplyText(const QString &dbusService, uint notificationId, const QString &text);

    ServerInfo *currentOwner() const;

    // Server only handles external application inhibitions but we still want the Inhibited property
    // expose the actual inhibition state for applications to check.
    void setInhibited(bool inhibited);

    bool externalInhibited() const;
    QList<Inhibition> externalInhibitions() const;
    void clearExternalInhibitions();

    bool m_valid = false;
    uint m_highestNotificationId = 1;

private slots:
    void onBroadcastNotification(const QMap<QString, QVariant> &properties);

private:
    void onServiceOwnershipLost(const QString &serviceName);
    void onInhibitionServiceUnregistered(const QString &serviceName);
    void onInhibitedChanged(); // emit DBus change signal

    bool m_dbusObjectValid = false;

    mutable QScopedPointer<ServerInfo> m_currentOwner;

    QDBusServiceWatcher *m_inhibitionWatcher = nullptr;
    QDBusServiceWatcher *m_notificationWatchers = nullptr;
    uint m_highestInhibitionCookie = 0;
    QHash<uint /*cookie*/, Inhibition> m_externalInhibitions;
    QHash<uint /*cookie*/, QString> m_inhibitionServices;

    bool m_inhibited = false;

    Notification m_lastNotification;

};

} // namespace NotificationManager
