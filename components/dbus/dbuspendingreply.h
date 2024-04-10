/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QVariantList>
#include <qqmlregistration.h>

#include "dbuserror.h"

class QDBusPendingCallWatcher;

namespace Plasma
{
class DBusConnection;
class DBusMessage;
class DBusPendingReply : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_DISABLE_COPY_MOVE(DBusPendingReply)

    Q_PROPERTY(bool isFinished READ isFinished NOTIFY finished)
    Q_PROPERTY(bool isError READ isError NOTIFY finished)
    Q_PROPERTY(bool isValid READ isValid NOTIFY finished)
    Q_PROPERTY(DBusError error READ error NOTIFY finished)
    Q_PROPERTY(QVariant value READ value NOTIFY finished)
    Q_PROPERTY(QVariantList values READ values NOTIFY finished)

public:
    explicit DBusPendingReply(const DBusConnection &connection, const DBusMessage &message, QObject *parent = nullptr);

    bool isFinished() const;
    bool isError() const;
    bool isValid() const;
    DBusError error() const;
    QVariant value() const;
    QVariantList values() const;

Q_SIGNALS:
    void finished();

private:
    void callInternal(const QDBusConnection &connection, const DBusMessage &message, const QByteArray &signature = {});

    bool m_isFinished = false;
    QDBusMessage m_reply;
    QVariantList m_arguments;
};
}
