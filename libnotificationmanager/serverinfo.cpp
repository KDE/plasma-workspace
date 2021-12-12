/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "serverinfo.h"

#include "server_p.h" // for notificationServiceName

#include "debug.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QDebug>

using namespace NotificationManager;

class Q_DECL_HIDDEN ServerInfo::Private
{
public:
    Private(ServerInfo *q);
    ~Private();

    void setStatus(ServerInfo::Status status);
    void setServerInformation(const QString &vendor, const QString &name, const QString &version, const QString &specVersion);

    void updateServerInformation();

    ServerInfo *q;

    ServerInfo::Status status = ServerInfo::Status::Unknown;

    QString vendor;
    QString name;
    QString version;
    QString specVersion;
};

ServerInfo::Private::Private(ServerInfo *q)
    : q(q)
{
}

ServerInfo::Private::~Private() = default;

void ServerInfo::Private::setStatus(ServerInfo::Status status)
{
    if (this->status != status) {
        this->status = status;
        Q_EMIT q->statusChanged(status);
    }
}

void ServerInfo::Private::setServerInformation(const QString &vendor, const QString &name, const QString &version, const QString &specVersion)
{
    if (this->vendor != vendor) {
        this->vendor = vendor;
        Q_EMIT q->vendorChanged(vendor);
    }
    if (this->name != name) {
        this->name = name;
        Q_EMIT q->nameChanged(name);
    }
    if (this->version != version) {
        this->version = version;
        Q_EMIT q->versionChanged(version);
    }
    if (this->specVersion != specVersion) {
        this->specVersion = specVersion;
        Q_EMIT q->specVersionChanged(specVersion);
    }
}

void ServerInfo::Private::updateServerInformation()
{
    // Check whether the service is running to avoid DBus-activating plasma_waitforname and getting stuck there.
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerPrivate::notificationServiceName())) {
        setStatus(ServerInfo::Status::NotRunning);
        setServerInformation({}, {}, {}, {});
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(ServerPrivate::notificationServiceName(),
                                                      QStringLiteral("/org/freedesktop/Notifications"),
                                                      QStringLiteral("org.freedesktop.Notifications"),
                                                      QStringLiteral("GetServerInformation"));
    auto call = QDBusConnection::sessionBus().asyncCall(msg);

    auto *watcher = new QDBusPendingCallWatcher(call, q);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, q, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QString, QString, QString, QString> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            qCWarning(NOTIFICATIONMANAGER) << "Failed to determine notification server information" << reply.error().message();
            // Should this still be "Running" as technically it is?
            // But if it is not even responding to this properly, who knows what it'll to with an actual notification
            setStatus(Status::Unknown);
            setServerInformation({}, {}, {}, {});
            return;
        }

        const QString name = reply.argumentAt(0).toString();
        const QString vendor = reply.argumentAt(1).toString();
        const QString version = reply.argumentAt(2).toString();
        const QString specVersion = reply.argumentAt(3).toString();

        setServerInformation(vendor, name, version, specVersion);
        setStatus(ServerInfo::Status::Running);
    });
}

ServerInfo::ServerInfo(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    auto *watcher =
        new QDBusServiceWatcher(ServerPrivate::notificationServiceName(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, [this]() {
        d->updateServerInformation();
    });

    d->updateServerInformation();
}

ServerInfo::~ServerInfo() = default;

ServerInfo::Status ServerInfo::status() const
{
    return d->status;
}

QString ServerInfo::vendor() const
{
    return d->vendor;
}

QString ServerInfo::name() const
{
    return d->name;
}

QString ServerInfo::version() const
{
    return d->version;
}

QString ServerInfo::specVersion() const
{
    return d->specVersion;
}
