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
/*!
    \qmlvaluetype dbusError
    \brief A D-Bus error.

    The type provides information about a D-Bus error, including whether the error is valid,
    the error message, and the error name. It can be used to handle and report errors that occur
    during D-Bus communication.

    \qml
        const promise = new Promise((resolve, reject) => {
            SessionBus.asyncCall(message, resolve, reject)
        }).then((result) => {
            console.log(result.error.isValid) // false
        }).catch((result) => {
            console.warn(result.error.name, result.error.message)
        })
    \endqml
*/
class DBusError
{
    Q_GADGET
    QML_VALUE_TYPE(dbusError)

    /*!
        \qmlproperty bool dbusError::isValid

        Indicates whether the D-Bus error is valid.
    */
    Q_PROPERTY(bool isValid READ isValid)

    /*!
        \qmlproperty string dbusError::message

        The detailed error message associated with the D-Bus error.
    */
    Q_PROPERTY(QString message READ message)

    /*!
        \qmlproperty string dbusError::message

        The name of the D-Bus error, which can be used to identify the type of error.
    */
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
