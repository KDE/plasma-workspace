/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbuserror.h"

namespace Plasma
{
DBusError::DBusError() = default;

DBusError::DBusError(const QDBusMessage &message)
    : m_isValid(message.type() == QDBusMessage::ErrorMessage || message.type() == QDBusMessage::InvalidMessage)
    , m_message(message.errorMessage())
    , m_name(message.errorName())
{
}

bool DBusError::isValid() const
{
    return m_isValid;
}

QString DBusError::message() const
{
    return m_message;
}

QString DBusError::name() const
{
    return m_name;
}
}

#include "moc_dbuserror.cpp"
