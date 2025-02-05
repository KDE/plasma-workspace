/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbusconnection.h"

#include <QDBusMessage>
#include <QDBusReply>
#include <QJSValue>
#include <QQmlEngine>
#include <QXmlStreamReader>

#include "dbusmessage.h"
#include "dbusplugin_debug.h"

using namespace Qt::StringLiterals;

namespace Plasma
{
DBusConnection::DBusConnection(QObject *parent)
    : QObject(parent)
    , m_connection(QDBusConnection(QString()))
{
}

DBusPendingReply *DBusConnection::asyncCall(const DBusMessage &message)
{
    return new DBusPendingReply(*this, message); // QML managed
}

void DBusConnection::asyncCall(const DBusMessage &message, const QJSValue &resolve, const QJSValue &reject)
{
    auto reply = new DBusPendingReply(*this, message);
    connect(reply, &DBusPendingReply::finished, this, [this, reply, resolve, reject] {
        QQmlEngine::setObjectOwnership(reply, QJSEngine::JavaScriptOwnership);
        const QJSValueList values{qjsEngine(this)->toScriptValue(reply)};
        QJSValue ret;
        if (reply->isValid()) {
            ret = resolve.call(values);
        } else {
            ret = reject.call(values);
        }
        if (ret.isError()) {
            qCWarning(DBUSPLUGIN_DEBUG) << ret.toString();
        }
    });
}

QByteArray DBusConnection::parseSignatureFromIntrospection(QStringView introspection, const DBusMessage &message)
{
    QXmlStreamReader reader{introspection};
    bool found = false;
    const QString interface = message.interface();
    const QString method = message.member();
    QString signature;
    while (!reader.atEnd() && !found) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == u"interface" && reader.attributes().value(u"name") == interface) {
            while (!reader.atEnd() && (reader.tokenType() != QXmlStreamReader::EndElement || reader.name() != u"interface")) {
                // Inside <interface>
                reader.readNext();
                if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == u"method" && reader.attributes().value(u"name") == method) {
                    // Collect signatures
                    while (!reader.atEnd() && (reader.tokenType() != QXmlStreamReader::EndElement || reader.name() != u"method")) {
                        // Inside <method>
                        reader.readNext();
                        if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == u"arg"
                            && reader.attributes().value(u"direction") == u"in") {
                            signature += reader.attributes().value(u"type");
                        }
                    }
                    found = true;
                    break;
                }
            }
        }
    }

    return '(' + std::move(signature).toLatin1() + ')';
}

SessionBusConnection::SessionBusConnection(QObject *parent)
    : DBusConnection(parent)
{
    m_connection = QDBusConnection::sessionBus();
}

SystemBusConnection::SystemBusConnection(QObject *parent)
    : DBusConnection(parent)
{
    m_connection = QDBusConnection::systemBus();
}
}

#include "moc_dbusconnection.cpp"
