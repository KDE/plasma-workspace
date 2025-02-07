/*
    SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "dbussignalwatcher.h"

#include <QDBusConnection>
#include <QJSValue>
#include <QQmlEngine>

#include "dbusdecoder.h"
#include "dbusplugin_debug.h"

using namespace Qt::StringLiterals;

namespace
{
constexpr auto dbusSignalHandlerPrefix = "dbus"_L1;
} // namespace

namespace Plasma
{
DBusSignalWatcher::DBusSignalWatcher(QObject *parent)
    : QObject(parent)
{
    QTextStream stream(&m_connectionPrefix);
    stream << this;
}

bool DBusSignalWatcher::enabled() const
{
    return m_enabled;
}

void DBusSignalWatcher::setEnabled(bool value)
{
    if (m_enabled == value) {
        return;
    }

    m_enabled = value;
    Q_EMIT enabledChanged();

    if (!isValid()) {
        return;
    }

    if (m_enabled) {
        connectToSignals();
    } else {
        disconnectFromSignals();
    }
}

QVariant DBusSignalWatcher::busType() const
{
    return QVariant::fromStdVariant(m_busType);
}

void DBusSignalWatcher::setBusType(const QVariant &type)
{
    if (auto v = std::get_if<BusType::Type>(&m_busType)) {
        if (type.metaType().id() == QMetaType::Int && *v == get<int>(type)) {
            return;
        }
    } else if (auto v = std::get_if<QString>(&m_busType)) {
        if (type.metaType().id() == QMetaType::QString && *v == get<QString>(type)) {
            return;
        }
    }

    if (isValid() && m_enabled) {
        disconnectFromSignals();
    }
    if (auto v = std::get_if<QString>(&m_busType)) {
        QDBusConnection::disconnectFromBus(m_connectionPrefix + *v);
    }

    if (type.metaType().id() == QMetaType::QString) {
        m_busType = get<QString>(type);
    } else {
        m_busType = static_cast<BusType::Type>(type.toInt());
    }
    Q_EMIT busTypeChanged();

    if (isValid() && m_enabled) {
        connectToSignals();
    }
}

QString DBusSignalWatcher::service() const
{
    return m_service;
}

void DBusSignalWatcher::setService(const QString &value)
{
    if (m_service == value) {
        return;
    }

    if (isValid() && m_enabled) {
        disconnectFromSignals();
    }

    m_service = value;
    Q_EMIT serviceChanged();

    if (isValid() && m_enabled) {
        connectToSignals();
    }
}

QString DBusSignalWatcher::path() const
{
    return m_path;
}

void DBusSignalWatcher::setPath(const QString &path)
{
    if (m_path == path) {
        return;
    }

    if (isValid() && m_enabled) {
        disconnectFromSignals();
    }

    m_path = path;
    Q_EMIT pathChanged();

    if (isValid() && m_enabled) {
        connectToSignals();
    }
}

QString DBusSignalWatcher::interface() const
{
    return m_interface;
}

void DBusSignalWatcher::setInterface(const QString &iface)
{
    if (m_interface == iface) {
        return;
    }

    if (isValid() && m_enabled) {
        disconnectFromSignals();
    }

    m_interface = iface;
    Q_EMIT interfaceChanged();

    if (isValid() && m_enabled) {
        connectToSignals();
    }
}

void DBusSignalWatcher::onReceivedSignal(const QDBusMessage &message)
{
    if (message.interface() != m_interface) {
        return;
    }

    QJSEngine *const engine = qjsEngine(this);
    const QString name = dbusSignalHandlerPrefix + message.member();
    const QJSValue value = engine->toScriptValue(this).property(name);
    if (!value.isCallable()) {
        qCWarning(DBUSPLUGIN_DEBUG) << "No signal handler for" << name;
        return;
    }

    QJSValueList args;
    args.reserve(message.arguments().size());
    for (const auto &arg : message.arguments()) {
        args << engine->toScriptValue(Decoder::dbusToVariant(arg));
    }
    const QJSValue ret = value.call(args);
    if (ret.isError()) {
        qCWarning(DBUSPLUGIN_DEBUG) << ret.toString();
    }
}

void DBusSignalWatcher::classBegin()
{
}

void DBusSignalWatcher::componentComplete()
{
    m_ready = true;

    if (!isValid() || !m_enabled) {
        return;
    }

    connectToSignals();
}

bool DBusSignalWatcher::isValid() const
{
    return m_ready && !m_service.isEmpty() && !m_path.isEmpty() && !m_interface.isEmpty();
}

QDBusConnection DBusSignalWatcher::connection() const
{
    if (auto v = std::get_if<BusType::Type>(&m_busType)) {
        return *v == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    }
    if (auto v = std::get_if<QString>(&m_busType)) {
        return QDBusConnection::connectToBus(*v, m_connectionPrefix + *v);
    }
    return QDBusConnection::sessionBus();
}

void DBusSignalWatcher::connectToSignals()
{
    Q_ASSERT(isValid());
    Q_ASSERT(m_enabled);
    connection().connect(m_service, m_path, m_interface, QString(), this, SLOT(onReceivedSignal(QDBusMessage)));
}

void DBusSignalWatcher::disconnectFromSignals()
{
    Q_ASSERT(isValid());
    connection().disconnect(m_service, m_path, m_interface, QString(), this, SLOT(onReceivedSignal(QDBusMessage)));
}
}

#include "moc_dbussignalwatcher.cpp"
