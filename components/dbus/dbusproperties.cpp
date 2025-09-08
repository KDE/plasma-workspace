/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbusproperties.h"

#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QXmlStreamReader>
#include <algorithm>

#include "dbusdecoder.h"
#include "dbusencoder.h"
#include "dbusplugin_debug.h"

using namespace Qt::StringLiterals;

namespace Plasma
{
DBusPropertyMap::DBusPropertyMap(QObject *parent)
    : QQmlPropertyMap(parent)
    , q(static_cast<DBusProperties *>(parent))
{
}

void DBusPropertyMap::update(const QString &key)
{
    if (key.isEmpty() || !q->isValid() || m_pendingGetProps.contains(key) || m_pendingSetProps.contains(key)) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(q->m_service, q->m_path, u"org.freedesktop.DBus.Properties"_s, u"Get"_s);
    message << q->m_interface << key;
    QDBusConnection conn = q->m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    QDBusPendingCall call = conn.asyncCall(message);
    auto watcher = m_pendingGetProps.emplace(key, std::make_unique<QDBusPendingCallWatcher>(call, this)).first->second.get();
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, key](QDBusPendingCallWatcher *watcher) {
        if (watcher->isError()) {
            qCWarning(DBUSPLUGIN_DEBUG).nospace() << watcher->reply().errorName() << ": " << watcher->reply().errorMessage();
            m_pendingGetProps.erase(key);
            return;
        }
        insert(key, Decoder::decode(watcher->reply()));
        m_pendingGetProps.erase(key);
    });
}

void DBusPropertyMap::updateAll()
{
    m_pendingGetProps.clear();
    m_pendingSetProps.clear();
    updateIntrospection();
    updateProperties();
}

void DBusPropertyMap::updateIntrospection()
{
    QDBusConnection conn = q->m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    QDBusMessage introspectMsg = QDBusMessage::createMethodCall(q->m_service, q->m_path, u"org.freedesktop.DBus.Introspectable"_s, u"Introspect"_s);
    m_introspectWatcher.reset(new QDBusPendingCallWatcher(conn.asyncCall(introspectMsg)));
    connect(m_introspectWatcher.get(), &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        if (watcher->isError()) {
            qCWarning(DBUSPLUGIN_DEBUG).nospace() << watcher->reply().errorName() << ": " << watcher->reply().errorMessage();
            return;
        }
        m_introspection = QDBusPendingReply<QString>(*watcher).value();
        m_introspectWatcher.reset(nullptr);
    });
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

void DBusPropertyMap::updateProperties(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (!interfaceName.isEmpty() && q->m_interface != interfaceName) {
        return;
    }

    if (changedProperties.empty() && invalidatedProperties.empty()) {
        Q_ASSERT(m_pendingGetProps.empty());
        Q_ASSERT(m_pendingSetProps.empty());
        QDBusMessage message = QDBusMessage::createMethodCall(q->m_service, q->m_path, u"org.freedesktop.DBus.Properties"_s, u"GetAll"_s);
        message << q->m_interface;
        QDBusConnection conn = q->m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
        QDBusPendingCall call = conn.asyncCall(message);
        m_refreshWatcher.reset(new QDBusPendingCallWatcher(call));
        connect(m_refreshWatcher.get(), &QDBusPendingCallWatcher::finished, this, &DBusPropertyMap::refreshCallback);
        return;
    }

    if (!changedProperties.empty()) {
        insert(QVariantHash(changedProperties.begin(), changedProperties.end()));
    }

    for (const QString &invalidKey : invalidatedProperties) {
        clear(invalidKey);
    }

    qCDebug(DBUSPLUGIN_DEBUG).noquote() << "Changed properties:" << changedProperties << "Invalidated properties:" << invalidatedProperties;
    Q_EMIT q->propertiesChanged(interfaceName, changedProperties, invalidatedProperties);
}

QVariant DBusPropertyMap::updateValue(const QString &key, const QVariant &input)
{
    Q_ASSERT_X(contains(key), Q_FUNC_INFO, qUtf8Printable(QString(u"Key " + key + u" does not exist.")));
    if (!q->isValid() || m_introspection.isEmpty()) {
        return value(key); // Keep the current value
    }

    if (auto it = m_pendingGetProps.find(key); it != m_pendingGetProps.end()) {
        m_pendingGetProps.erase(it);
    }

    if (auto it = m_pendingSetProps.find(key); it != m_pendingSetProps.end()) {
        m_pendingSetProps.erase(it);
    }

    QDBusMessage message = QDBusMessage::createMethodCall(q->m_service, q->m_path, u"org.freedesktop.DBus.Properties"_s, u"Set"_s);
    QVariantList arguments = Encoder::encode(QVariantList{input}, propertySignature(key).constData()).toList();
    arguments.prepend(key);
    arguments.prepend(q->m_interface);
    arguments[2] = QVariant::fromValue(QDBusVariant(arguments[2]));
    message.setArguments(arguments);
    QDBusConnection conn = q->m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    QDBusPendingCall call = conn.asyncCall(message);
    std::unique_ptr<QDBusPendingCallWatcher, QScopedPointerDeleteLater> watcher;
    watcher.reset(new QDBusPendingCallWatcher(call));
    connect(watcher.get(), &QDBusPendingCallWatcher::finished, this, [this, key, oldValue = value(key)](QDBusPendingCallWatcher *watcher) {
        if (watcher->isError()) {
            qCWarning(DBUSPLUGIN_DEBUG).nospace() << watcher->reply().errorName() << ": " << watcher->reply().errorMessage();
            insert(key, oldValue);
        }
        m_pendingSetProps.erase(key);
    });
    m_pendingSetProps.emplace(key, std::move(watcher));

    return input;
}

QByteArray DBusPropertyMap::propertySignature(QStringView key)
{
    QXmlStreamReader reader{m_introspection};
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == u"interface" && reader.attributes().value(u"name") == q->m_interface) {
            while (!reader.atEnd() && (reader.tokenType() != QXmlStreamReader::EndElement || reader.name() != u"interface")) {
                // Inside <interface>
                reader.readNext();
                if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == u"property" && reader.attributes().value(u"name") == key) {
                    // Inside <property>
                    return '(' + reader.attributes().value(u"type").toLatin1() + ')';
                }
            }
        }
    }
    return "()"_ba;
}

void DBusPropertyMap::refreshCallback(QDBusPendingCallWatcher *watcher)
{
    QScopeGuard guard([this] {
        Q_EMIT q->refreshed();
        m_refreshWatcher.reset(nullptr);
    });

    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (!reply.isValid()) {
        return;
    }

    QVariantMap properties = reply.value();
    const QStringList lastKeys = keys();
    if (properties.empty() && lastKeys.empty()) [[unlikely]] {
        return;
    }

    for (auto it = properties.begin(); it != properties.end(); it = std::next(it)) {
        it.value() = Decoder::dbusToVariant(it.value());
    }

    QStringList invalidatedProperties;
    std::ranges::copy_if(lastKeys, std::back_inserter(invalidatedProperties), [&properties](const QString &key) {
        return !properties.contains(key);
    });

    updateProperties(q->m_interface, properties, invalidatedProperties);
}

DBusProperties::DBusProperties(QObject *parent)
    : QObject(parent)
    , m_properties(new DBusPropertyMap(this))
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
        m_properties->updateIntrospection();
        m_properties->updateProperties();
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
        m_properties->updateIntrospection();
        m_properties->updateProperties();
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
        m_properties->updateIntrospection();
        m_properties->updateProperties();
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
        m_properties->updateProperties();
    }
}

DBusPropertyMap *DBusProperties::properties() const
{
    return m_properties;
}

void DBusProperties::resetProperties()
{
    DBusPropertyMap *oldPropertyMap = std::exchange(m_properties, new DBusPropertyMap(this));
    Q_EMIT propertyMapChanged();
    delete oldPropertyMap;
}

void DBusProperties::update(const QString &key)
{
    m_properties->update(key);
}

void DBusProperties::updateAll()
{
    m_properties->updateAll();
}

void DBusProperties::updateProperties(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    m_properties->updateProperties(interfaceName, changedProperties, invalidatedProperties);
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

    m_properties->updateIntrospection();
    m_properties->updateProperties();
    connectToPropertiesChangedSignal();
}

bool DBusProperties::isValid() const
{
    return m_ready && !m_service.isEmpty() && !m_path.isEmpty() && !m_interface.isEmpty();
}
}

#include "moc_dbusproperties.cpp"
