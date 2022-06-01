/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDBusContext>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <memory>

#include "notification.h"

class QDBusServiceWatcher;

struct Inhibition {
    QString desktopEntry;
    QString applicationName;
    // QString applicationIconName;
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
    uint Notify(const QString &app_name,
                uint replaces_id,
                const QString &app_icon,
                const QString &summary,
                const QString &body,
                const QStringList &actions,
                const QVariantMap &hints,
                int timeout);
    void CloseNotification(uint id);
    QStringList GetCapabilities() const;
    QString GetServerInformation(QString &vendor, QString &version, QString &specVersion) const;

    // Inhibitions
    uint Inhibit(const QString &desktop_entry, const QString &reason, const QVariantMap &hints);
    void UnInhibit(uint cookie);
    bool inhibited() const; // property getter

    // Notifition watcher
    void RegisterWatcher();
    void UnRegisterWatcher();

    void InvokeAction(uint id, const QString &actionKey);

Q_SIGNALS:
    // DBus
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);
    void ActivationToken(uint id, const QString &xdgActivationToken);
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
    void sendReplyText(const QString &dbusService, uint notificationId, const QString &text, Notifications::InvokeBehavior behavior);

    ServerInfo *currentOwner() const;

    // Server only handles external application inhibitions but we still want the Inhibited property
    // expose the actual inhibition state for applications to check.
    void setInhibited(bool inhibited);

    bool externalInhibited() const;
    QList<Inhibition> externalInhibitions() const;
    void clearExternalInhibitions();

    bool m_valid = false;
    uint m_highestNotificationId = 1;

private Q_SLOTS:
    void onBroadcastNotification(const QMap<QString, QVariant> &properties);

private:
    friend class Server;
    void onServiceOwnershipLost(const QString &serviceName);
    void onInhibitionServiceUnregistered(const QString &serviceName);
    void onInhibitedChanged(); // Q_EMIT DBus change signal

    bool m_dbusObjectValid = false;

    mutable std::unique_ptr<ServerInfo> m_currentOwner;

    QDBusServiceWatcher *m_inhibitionWatcher = nullptr;
    QDBusServiceWatcher *m_notificationWatchers = nullptr;
    uint m_highestInhibitionCookie = 0;
    QHash<uint /*cookie*/, Inhibition> m_externalInhibitions;
    QHash<uint /*cookie*/, QString> m_inhibitionServices;

    bool m_inhibited = false;

    Notification m_lastNotification;
};

} // namespace NotificationManager
