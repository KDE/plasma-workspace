/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbusreply.h"
#include "dbusdecoder.h"
#include "dbuspendingreply.h"

namespace Plasma
{
DBusReply::DBusReply()
{
}

DBusReply::DBusReply(DBusPendingReply *pendingReply)
    : m_message(pendingReply ? pendingReply->m_reply : QDBusMessage())
    , m_arguments(pendingReply ? pendingReply->m_arguments : QVariantList())
{
}

DBusReply::DBusReply(const QDBusMessage &reply)
    : m_message(reply)
    , m_arguments(Decoder::decode(reply))
{
}

bool DBusReply::isValid() const
{
    return m_message.type() == QDBusMessage::ReplyMessage;
}

DBusError DBusReply::error() const
{
    return DBusError(m_message);
}

QVariant DBusReply::value() const
{
    return m_arguments.empty() ? QVariant() : m_arguments[0];
}

QVariantList DBusReply::values() const
{
    return m_arguments;
}
}

#include "moc_dbusreply.cpp"
