/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbusproperties.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

using namespace Qt::StringLiterals;

namespace Plasma
{
DBusProperties::DBusProperties(QObject *parent)
    : QObject(parent)
    , m_properties(new QQmlPropertyMap(this))
{
}

BusType::Type DBusProperties::busType() const
{
    return m_busType;
}

void DBusProperties::setBusType(BusType::Type type)
{
    if (m_busType == type) {
        return;
    }

    if (isValid()) {
        disconnectFromPropertiesChangedSignal();
        resetProperties();
    }

    m_busType = type;
    Q_EMIT busTypeChanged();

    if (isValid()) {
        updateProperties();
        connectToPropertiesChangedSignal();
    }
}

QString DBusProperties::service() const
{
    return m_service;
}

void DBusProperties::setService(const QString &value)
{
    if (m_service == value) {
        return;
    }

    if (isValid()) {
        disconnectFromPropertiesChangedSignal();
        resetProperties();
    }

    m_service = value;
    Q_EMIT serviceChanged();

    if (isValid()) {
        updateProperties();
        connectToPropertiesChangedSignal();
    }
}

QString DBusProperties::path() const
{
    return m_path;
}

void DBusProperties::setPath(const QString &path)
{
    if (m_path == path) {
        return;
    }

    if (isValid()) {
        disconnectFromPropertiesChangedSignal();
        resetProperties();
    }

    m_path = path;
    Q_EMIT pathChanged();

    if (isValid()) {
        updateProperties();
        connectToPropertiesChangedSignal();
    }
}

QString DBusProperties::interface() const
{
    return m_interface;
}

void DBusProperties::setInterface(const QString &iface)
{
    if (m_interface == iface) {
        return;
    }

    if (isValid()) {
        resetProperties();
    }

    m_interface = iface;
    Q_EMIT interfaceChanged();

    if (isValid()) {
        updateProperties();
    }
}

QQmlPropertyMap *DBusProperties::properties() const
{
    return m_properties;
}

void DBusProperties::resetProperties()
{
    if (m_refreshWatcher) {
        delete m_refreshWatcher;
    }
    QQmlPropertyMap *oldPropertyMap = std::exchange(m_properties, new QQmlPropertyMap(this));
    Q_EMIT propertyMapChanged();
    delete oldPropertyMap;
}

void DBusProperties::update(const QString &key)
{
    if (key.isEmpty() || !isValid() || m_updateKeyWatchers.contains(key)) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(m_service, m_path, u"org.freedesktop.DBus.Properties"_s, u"Get"_s);
    message << m_interface << key;
    QDBusConnection conn = m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    QDBusPendingCall call = conn.asyncCall(message);
    auto watcher = m_updateKeyWatchers.emplace(key, new QDBusPendingCallWatcher(call, this)).first->second;
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, key](QDBusPendingCallWatcher *watcher) {
        std::size_t erased = m_updateKeyWatchers.erase(key);
        Q_ASSERT(erased > 0);
        QDBusPendingReply<QVariant> reply = *watcher;
        if (!reply.isValid()) {
            watcher->deleteLater();
            return;
        }
        m_properties->insert(key, reply.value());
        watcher->deleteLater();
    });
}

void DBusProperties::updateAll()
{
    updateProperties();
}

void DBusProperties::updateProperties(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (!interfaceName.isEmpty() && m_interface != interfaceName) {
        return;
    }

    if (changedProperties.empty() && invalidatedProperties.empty()) {
        QDBusMessage message = QDBusMessage::createMethodCall(m_service, m_path, u"org.freedesktop.DBus.Properties"_s, u"GetAll"_s);
        message << m_interface;
        QDBusConnection conn = m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
        QDBusPendingCall call = conn.asyncCall(message);
        if (m_refreshWatcher) {
            delete m_refreshWatcher;
        }
        m_refreshWatcher = new QDBusPendingCallWatcher(call, this);
        connect(m_refreshWatcher.get(), &QDBusPendingCallWatcher::finished, this, &DBusProperties::refreshCallback);
        return;
    }

    if (!changedProperties.empty()) {
        m_properties->insert(QVariantHash(changedProperties.begin(), changedProperties.end()));
    }

    for (const QString &invalidKey : invalidatedProperties) {
        m_properties->clear(invalidKey);
    }
}

void DBusProperties::classBegin()
{
}

void DBusProperties::componentComplete()
{
    m_ready = true;

    if (!isValid()) {
        return;
    }

    Q_ASSERT(m_properties->isEmpty());
    Q_ASSERT(!m_refreshWatcher);

    updateProperties();
    connectToPropertiesChangedSignal();
}

bool DBusProperties::isValid() const
{
    return m_ready && !m_service.isEmpty() && !m_path.isEmpty() && !m_interface.isEmpty();
}

void DBusProperties::refreshCallback(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (!reply.isValid()) {
        watcher->deleteLater();
        return;
    }

    const QVariantMap properties = reply.value();
    const QStringList lastKeys = m_properties->keys();
    if (properties.empty() && lastKeys.empty()) [[unlikely]] {
        return;
    }

    QStringList invalidatedProperties;
    std::copy_if(lastKeys.begin(), lastKeys.end(), std::back_inserter(invalidatedProperties), [&properties](const QString &key) {
        return properties.contains(key);
    });

    updateProperties(m_interface, properties, invalidatedProperties);
    watcher->deleteLater();
}

void DBusProperties::connectToPropertiesChangedSignal()
{
    Q_ASSERT_X(isValid(), Q_FUNC_INFO, qPrintable(QString(u"Service: " + m_service + u" Path: " + m_path + u" Interface: " + m_interface)));
    QDBusConnection conn = m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    conn.connect(m_service,
                 m_path,
                 u"org.freedesktop.DBus.Properties"_s,
                 u"PropertiesChanged"_s,
                 this,
                 SLOT(updateProperties(QString, QVariantMap, QStringList)));
}

void DBusProperties::disconnectFromPropertiesChangedSignal()
{
    Q_ASSERT_X(isValid(), Q_FUNC_INFO, qPrintable(QString(u"Service: " + m_service + u" Path: " + m_path + u" Interface: " + m_interface)));
    QDBusConnection conn = m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    conn.disconnect(m_service,
                    m_path,
                    u"org.freedesktop.DBus.Properties"_s,
                    u"PropertiesChanged"_s,
                    this,
                    SLOT(updateProperties(QString, QVariantMap, QStringList)));
}
}

#include "moc_dbusproperties.cpp"
