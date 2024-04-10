/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbusmethodcall.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QMetaType>
#include <QXmlStreamReader>

#include "dbusdecoder.h"
#include "dbusencoder.h"

using namespace Qt::StringLiterals;

DBusMethodCall::DBusMethodCall(QObject *parent)
    : QObject(parent)
{
}

DBusMethodCall::~DBusMethodCall()
{
}

BusType::Type DBusMethodCall::busType() const
{
    return m_busType;
}

void DBusMethodCall::setBusType(BusType::Type type)
{
    if (m_busType == type) {
        return;
    }

    m_busType = type;
    reset();
    Q_EMIT busTypeChanged();
}

QString DBusMethodCall::service() const
{
    return m_service;
}

void DBusMethodCall::setService(const QString &value)
{
    if (m_service == value) {
        return;
    }

    m_service = value;
    reset();
    Q_EMIT serviceChanged();
}

QString DBusMethodCall::objectPath() const
{
    return m_objectPath;
}

void DBusMethodCall::setObjectPath(const QString &path)
{
    if (m_objectPath == path) {
        return;
    }

    m_objectPath = path;
    if (*m_objectPath.crbegin() == QLatin1Char('/')) {
        m_objectPath.chop(1);
    }
    reset();
    Q_EMIT objectPathChanged();
}

QString DBusMethodCall::interface() const
{
    return m_interface;
}

void DBusMethodCall::setInterface(const QString &iface)
{
    if (m_interface == iface) {
        return;
    }

    m_interface = iface;
    reset();
    Q_EMIT ifaceChanged();
}

QString DBusMethodCall::method() const
{
    return m_method;
}

void DBusMethodCall::setMethod(const QString &name)
{
    if (m_method == name) {
        return;
    }

    m_method = name;
    reset();
    Q_EMIT methodChanged();
}

QString DBusMethodCall::inSignature() const
{
    return m_inSignature.value_or(QString());
}

void DBusMethodCall::setInSignature(const QString &sig)
{
    if (m_inSignature == sig) {
        return;
    }

    m_inSignature = sig;
    reset();
    Q_EMIT inSignatureChanged();
}

void DBusMethodCall::resetInSignature()
{
    if (!m_inSignature.has_value()) {
        return;
    }

    reset();
    Q_EMIT inSignatureChanged();
}

QJSValue DBusMethodCall::arguments() const
{
    return m_arguments;
}

void DBusMethodCall::setArguments(const QJSValue &args)
{
    if (m_arguments.strictlyEquals(args)) {
        return;
    }

    m_arguments = args;
    reset();
    m_isArgumentsSet = m_arguments.isArray() || m_arguments.isBool() || m_arguments.isNumber() || m_arguments.isString() || m_arguments.isUrl();
    Q_EMIT argumentsChanged();
}

void DBusMethodCall::call()
{
    if (!m_ready || m_service.isEmpty() || m_objectPath.isEmpty() || m_interface.isEmpty() || m_method.isEmpty()) {
        return;
    }

    if (m_isArgumentsSet && m_argParseResult.isNull()) {
        parseArguments();
        return;
    }

    internalCall();
}

void DBusMethodCall::classBegin()
{
}

void DBusMethodCall::componentComplete()
{
    m_ready = true;
}

void DBusMethodCall::parseArguments()
{
    Q_ASSERT(m_ready);
    Q_ASSERT(m_argParseResult.isNull());
    Q_ASSERT(m_isArgumentsSet);

    if (!m_inSignature.has_value()) {
        // Auto deduct
        if (m_service.isEmpty() || m_objectPath.isEmpty()) {
            return;
        }

        QDBusMessage message = QDBusMessage::createMethodCall(m_service, m_objectPath, u"org.freedesktop.DBus.Introspectable"_s, u"Introspect"_s);
        QDBusConnection conn = m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
        m_argParseWatcher.reset(new QDBusPendingCallWatcher(conn.asyncCall(message)));
        connect(m_argParseWatcher.get(), &QDBusPendingCallWatcher::finished, this, [this] {
            std::unique_ptr<void, std::function<void(void *)>> guard{this, [this](auto) {
                                                                         internalCall();
                                                                         m_argParseWatcher.reset();
                                                                     }};
            QDBusReply<QString> reply = *m_argParseWatcher.get();
            if (!reply.isValid()) {
                m_argParseWatcher.reset();
                return;
            }
            parseSignatureFromIntrospection(reply.value());
            if (m_inSignature.has_value()) {
                m_argParseResult = Encoder::encode(m_arguments.toVariant(), m_inSignature->toLatin1().constData());
            }
        });
    } else {
        m_argParseResult = Encoder::encode(m_arguments.toVariant(), m_inSignature->toLatin1().constData());
        internalCall();
    }
}

void DBusMethodCall::parseSignatureFromIntrospection(QStringView introspection)
{
    Q_ASSERT(!m_interface.isEmpty());
    Q_ASSERT(!m_method.isEmpty());
    Q_ASSERT(!m_inSignature.has_value());

    QXmlStreamReader reader{introspection};
    bool found = false;
    QString signature;
    while (!reader.atEnd() && !found) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "interface"_L1 && reader.attributes().value("name"_L1) == m_interface) {
            while (!reader.atEnd() && (reader.tokenType() != QXmlStreamReader::EndElement || reader.name() != "interface"_L1)) {
                // Inside <interface>
                reader.readNext();
                if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "method"_L1 && reader.attributes().value("name"_L1) == m_method) {
                    // Collect signatures
                    while (!reader.atEnd() && (reader.tokenType() != QXmlStreamReader::EndElement || reader.name() != "method"_L1)) {
                        // Inside <method>
                        reader.readNext();
                        if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "arg"_L1
                            && reader.attributes().value("direction"_L1) == "in"_L1) {
                            signature += reader.attributes().value("type"_L1);
                        }
                    }
                    found = true;
                    break;
                }
            }
        }
    }

    if (!signature.isEmpty()) {
        m_inSignature = u'(' + signature + u')';
    }
}

void DBusMethodCall::internalCall()
{
    Q_ASSERT(m_ready && !m_service.isEmpty() && !m_objectPath.isEmpty() && !m_interface.isEmpty() && !m_method.isEmpty());

    QDBusMessage message = QDBusMessage::createMethodCall(m_service, m_objectPath, m_interface, m_method);
    if (m_argParseResult.isValid()) {
        if (m_argParseResult.typeId() == QMetaType::QVariantList) {
            message.setArguments(get<QVariantList>(m_argParseResult));
        } else {
            message << m_argParseResult;
        }
    }

    QDBusConnection conn = m_busType == BusType::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    m_watcher.reset(new QDBusPendingCallWatcher(conn.asyncCall(message)));
    connect(m_watcher.get(), &QDBusPendingCallWatcher::finished, this, [this] {
        QDBusMessage message = m_watcher->reply();
        if (m_watcher->isError()) {
            Q_EMIT error(message.errorName(), message.errorMessage());
        } else {
            QVariantList reply = message.arguments();
            for (QVariant &r : reply) {
                r = Decoder::dbusToVariant(r);
            }
            Q_EMIT success(reply);
        }
        m_watcher.reset();
    });
}

void DBusMethodCall::reset()
{
    m_argParseResult.clear();
    m_argParseWatcher.reset();
    m_watcher.reset();
}

#include "moc_dbusmethodcall.cpp"
