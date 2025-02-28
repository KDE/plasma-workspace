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

    /*!
        \qmlmethod DBusPendingReply org.kde.plasma.workspace.dbus::DBusConnection::asyncCall(message)

        Asynchronously calls the D-Bus method described by the given message.
        \a message The DBusMessage object that contains information about the D-Bus method call.

        \qml
        SessionBus.asyncCall({
            "service": "org.kde.KSplash",
            "path": "/KSplash",
            "iface": "org.kde.KSplash",
            "member": "setStage",
            "signature": "s",
            "arguments": [new DBus.string("break")],
        })
        \endqml
    */
    Q_INVOKABLE DBusPendingReply *asyncCall(const DBusMessage &message);

    /*!
        \qmlmethod void org.kde.plasma.workspace.dbus::DBusConnection::asyncCall(message, resolve, reject)

        Asynchronously calls the D-Bus method described by the given message with JavaScript callback functions.
        \a message The DBusMessage object that contains information about the D-Bus method call.
        \a resolve A QJSValue representing the JavaScript callback function to be called when the asynchronous call succeeds.
        \a reject A QJSValue representing the JavaScript callback function to be called when the asynchronous call fails.

        \qml
        const promise = new Promise((resolve, reject) => {
            SessionBus.asyncCall({
                "service": "org.kde.KSplash",
                "path": "/KSplash",
                "iface": "org.kde.KSplash",
                "member": "setStage",
                "signature": "s",
                "arguments": [new DBus.string("break")],
            }, resolve, reject)
        }).then((result) => {
            ...
        }).catch((error) => {
            ...
        })
        \endqml
    */
    Q_INVOKABLE void asyncCall(const DBusMessage &message, const QJSValue &resolve, const QJSValue &reject);

    static QByteArray parseSignatureFromIntrospection(QStringView introspection, const DBusMessage &message);

    operator QDBusConnection() const
    {
        return m_connection;
    }

protected:
    QDBusConnection m_connection;
};

/*!
    \qmltype SessionBus
    \inherits DBusConnection
    \nativetype SessionBusConnection
    \inqmlmodule org.kde.plasma.workspace.dbus

    \brief Provides a connection to the session bus.

    This class inherits from DBusConnection and provides a way to interact with the session bus.

    \qml
        SessionBus.asyncCall(message)
    \endqml

    \sa SystemBus
*/
class SessionBusConnection : public DBusConnection
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SessionBus)
    QML_SINGLETON

public:
    explicit SessionBusConnection(QObject *parent = nullptr);
};

/*!
    \qmltype SystemBus
    \inherits DBusConnection
    \nativetype SystemBusConnection
    \inqmlmodule org.kde.plasma.workspace.dbus

    \brief Provides a connection to the system bus.

    This class inherits from DBusConnection and provides a way to interact with the system bus.

    \qml
        SystemBus.asyncCall(message)
    \endqml

    \sa SessionBus
*/
class SystemBusConnection : public DBusConnection
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SystemBus)
    QML_SINGLETON

public:
    explicit SystemBusConnection(QObject *parent = nullptr);
};
}
