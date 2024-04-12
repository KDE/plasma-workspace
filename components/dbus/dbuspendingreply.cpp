/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbuspendingreply.h"

#include <QDBusPendingCallWatcher>
#include <QDBusReply>

#include "dbusconnection.h"
#include "dbusdecoder.h"
#include "dbusencoder.h"
#include "dbusmessage.h"

using namespace Qt::StringLiterals;

namespace Plasma
{
DBusPendingReply::DBusPendingReply(const DBusConnection &connection, const DBusMessage &message, QObject *parent)
    : QObject(parent)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(message.service(), message.path(), message.interface(), message.member());
    if (QVariantList arguments = message.arguments(); arguments.empty()) {
        callInternal(static_cast<QDBusConnection>(connection), message);
    } else if (const QString signature = message.signature(); signature.isEmpty()) {
        QDBusMessage introspectMsg =
            QDBusMessage::createMethodCall(message.service(), message.path(), u"org.freedesktop.DBus.Introspectable"_s, u"Introspect"_s);
        auto argParseWatcher = new QDBusPendingCallWatcher(static_cast<QDBusConnection>(connection).asyncCall(introspectMsg));
        connect(argParseWatcher,
                &QDBusPendingCallWatcher::finished,
                this,
                [this, conn = static_cast<QDBusConnection>(connection), message, arguments](QDBusPendingCallWatcher *watcher) {
                    watcher->deleteLater();
                    callInternal(conn, message, DBusConnection::parseSignatureFromIntrospection(QDBusReply<QString>(*watcher).value(), message));
                });
    } else {
        callInternal(static_cast<QDBusConnection>(connection), message, signature.toLatin1());
    }
}

bool DBusPendingReply::isFinished() const
{
    return m_isFinished;
}

bool DBusPendingReply::isError() const
{
    return m_isFinished && m_reply.type() != QDBusMessage::ReplyMessage;
}

bool DBusPendingReply::isValid() const
{
    return !m_isFinished || m_reply.type() == QDBusMessage::ReplyMessage;
}

DBusError DBusPendingReply::error() const
{
    return DBusError(m_reply);
}

QVariant DBusPendingReply::value() const
{
    return m_arguments.empty() ? QVariant() : m_arguments[0];
}

QVariantList DBusPendingReply::values() const
{
    return m_arguments;
}

void DBusPendingReply::callInternal(const QDBusConnection &connection, const DBusMessage &message, const QByteArray &signature)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(message.service(), message.path(), message.interface(), message.member());
    if (!signature.isEmpty()) {
        msg.setArguments(Encoder::encode(message.arguments(), signature.constData()).toList());
    }
    auto watcher = new QDBusPendingCallWatcher(connection.asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        m_isFinished = true;
        m_reply = watcher->reply();
        m_arguments = Decoder::decode(m_reply);
        Q_EMIT finished();
        delete watcher;
    });
}
}

#include "moc_dbuspendingreply.cpp"
