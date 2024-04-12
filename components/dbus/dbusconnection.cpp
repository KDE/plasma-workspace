/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbusconnection.h"

#include <QDBusMessage>
#include <QDBusReply>
#include <QXmlStreamReader>

#include "dbusencoder.h"
#include "dbusmessage.h"

using namespace Qt::StringLiterals;

namespace Plasma
{
DBusConnection::DBusConnection(QObject *parent)
    : QObject(parent)
    , m_connection(QDBusConnection(QString()))
{
}

DBusReply DBusConnection::call(const DBusMessage &message)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(message.service(), message.path(), message.interface(), message.member());
    QVariantList arguments = message.arguments();
    if (arguments.empty()) {
        return DBusReply(m_connection.call(msg));
    }

    if (const QString signature = message.signature(); signature.isEmpty()) {
        QDBusMessage introspectMsg =
            QDBusMessage::createMethodCall(message.service(), message.path(), u"org.freedesktop.DBus.Introspectable"_s, u"Introspect"_s);
        QDBusReply<QString> introspection = m_connection.call(introspectMsg);
        if (introspection.isValid()) {
            msg.setArguments(Encoder::encode(arguments, parseSignatureFromIntrospection(introspection.value(), message).constData()).toList());
        }
    } else {
        msg.setArguments(Encoder::encode(arguments, signature.toLatin1().constData()).toList());
    }

    return DBusReply(m_connection.call(msg));
}

DBusPendingReply *DBusConnection::asyncCall(const DBusMessage &message)
{
    return new DBusPendingReply(*this, message); // QML managed
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
        if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "interface"_L1 && reader.attributes().value("name"_L1) == interface) {
            while (!reader.atEnd() && (reader.tokenType() != QXmlStreamReader::EndElement || reader.name() != "interface"_L1)) {
                // Inside <interface>
                reader.readNext();
                if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "method"_L1 && reader.attributes().value("name"_L1) == method) {
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
