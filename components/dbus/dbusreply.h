/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusMessage>
#include <QVariantList>
#include <qqmlregistration.h>

#include "dbuserror.h"
#include "dbusmessage.h"

namespace Plasma
{
class DBusPendingReply;
class DBusReply
{
    Q_GADGET
    QML_VALUE_TYPE(dbusReply)
    QML_CONSTRUCTIBLE_VALUE

    Q_PROPERTY(bool isValid READ isValid)
    Q_PROPERTY(DBusError error READ error)
    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(QVariantList values READ values)

public:
    explicit DBusReply();
    Q_INVOKABLE explicit DBusReply(DBusPendingReply *pendingReply);
    explicit DBusReply(const QDBusMessage &reply);

    bool isValid() const;
    DBusError error() const;
    QVariant value() const;
    QVariantList values() const;

private:
    QDBusMessage m_message;
    QVariantList m_arguments;
};
}
