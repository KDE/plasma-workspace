/*
 *   SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "waydroidapplicationdbusclient.h"

#include <QDBusConnection>

using namespace Qt::StringLiterals;

WaydroidApplicationDBusClient::WaydroidApplicationDBusClient(const QDBusObjectPath &objectPath, QObject *parent)
    : QObject{parent}
    , m_objectPath{objectPath}
    , m_interface{new OrgKdePlasmashellWaydroidApplicationInterface{u"org.kde.plasmashell"_s, objectPath.path(), QDBusConnection::sessionBus(), this}}
    , m_watcher{new QDBusServiceWatcher{u"org.kde.plasmashell"_s, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this}}
{
    // Check if the service is already running
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(u"org.kde.plasmashell"_s)) {
        m_connected = true;
        if (m_interface->isValid()) {
            connectSignals();
        }
    }

    connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, [this](const QString &service, const QString &oldOwner, const QString &newOwner) {
        if (service == u"org.kde.plasmashell"_s) {
            if (newOwner.isEmpty()) {
                // Service stopped
                m_connected = false;
            } else if (oldOwner.isEmpty()) {
                // Service started
                m_connected = true;
                if (m_interface->isValid()) {
                    connectSignals();
                }
            }
        }
    });
}

void WaydroidApplicationDBusClient::connectSignals()
{
    // Initialize properties
    updateName();
    updatePackageName();
}

QString WaydroidApplicationDBusClient::name() const
{
    return m_name;
}

QString WaydroidApplicationDBusClient::packageName() const
{
    return m_packageName;
}

QDBusObjectPath WaydroidApplicationDBusClient::objectPath() const
{
    return m_objectPath;
}

void WaydroidApplicationDBusClient::updateName()
{
    if (!m_connected) {
        return;
    }

    auto reply = m_interface->name();
    auto watcher = new QDBusPendingCallWatcher(reply, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](auto watcher) {
        QDBusPendingReply<QString> reply = *watcher;
        if (!reply.isValid()) {
            qDebug() << "WaydroidApplicationDBusClient: Failed to fetch name:" << reply.error().message();
            watcher->deleteLater();
            return;
        }

        const auto name = reply.argumentAt<0>();

        if (m_name != name) {
            m_name = name;
            Q_EMIT nameChanged();
        }

        watcher->deleteLater();
    });
}

void WaydroidApplicationDBusClient::updatePackageName()
{
    if (!m_connected) {
        return;
    }

    auto reply = m_interface->packageName();
    auto watcher = new QDBusPendingCallWatcher(reply, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](auto watcher) {
        QDBusPendingReply<QString> reply = *watcher;
        if (!reply.isValid()) {
            qDebug() << "WaydroidApplicationDBusClient: Failed to fetch packageName:" << reply.error().message();
            watcher->deleteLater();
            return;
        }

        const auto packageName = reply.argumentAt<0>();

        if (m_packageName != packageName) {
            m_packageName = packageName;
            Q_EMIT packageNameChanged();
        }

        watcher->deleteLater();
    });
}