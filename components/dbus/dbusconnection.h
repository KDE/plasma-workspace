/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusConnection>
#include <QVariantList>
#include <qqmlregistration.h>

#include "dbuspendingreply.h"

class QJSValue;

namespace Plasma
{
class DBusMessage;

class DBusConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DBusConnection)

public:
    explicit DBusConnection(QObject *parent = nullptr);

    Q_INVOKABLE DBusPendingReply *asyncCall(const DBusMessage &message);
    Q_INVOKABLE void asyncCall(const DBusMessage &message, const QJSValue &resolve, const QJSValue &reject);

    static QByteArray parseSignatureFromIntrospection(QStringView introspection, const DBusMessage &message);

    operator QDBusConnection() const
    {
        return m_connection;
    }

protected:
    QDBusConnection m_connection;
};

class SessionBusConnection : public DBusConnection
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SessionBus)
    QML_SINGLETON

public:
    explicit SessionBusConnection(QObject *parent = nullptr);
};

class SystemBusConnection : public DBusConnection
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SystemBus)
    QML_SINGLETON

public:
    explicit SystemBusConnection(QObject *parent = nullptr);
};
}
