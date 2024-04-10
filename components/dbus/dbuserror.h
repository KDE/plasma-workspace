/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusError>
#include <QDBusMessage>
#include <qqmlregistration.h>

namespace Plasma
{
class DBusError
{
    Q_GADGET
    QML_VALUE_TYPE(dbusError)

    Q_PROPERTY(bool isValid READ isValid)
    Q_PROPERTY(QString message READ message)
    Q_PROPERTY(QString name READ name)

public:
    explicit DBusError();
    explicit DBusError(const QDBusMessage &message);

    bool isValid() const;
    QString message() const;
    QString name() const;

private:
    bool m_isValid = false;
    QString m_message;
    QString m_name;
};
}
