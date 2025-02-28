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

/*!
    \qmltype DBusConnection
    \inherits QtObject
    \inqmlmodule org.kde.plasma.workspace.dbus

    \brief The DBusConnection class provides an interface to interact with the D-Bus system.

    This class allows for asynchronous method calls on D-Bus services, and provides the ability to
    automatically parse method signatures from introspection data.

    \sa SessionBus, SystemBus
*/
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
        // Get the current wallpaper
        const pendingReply = SessionBus.asyncCall({
            "service": "org.kde.plasmashell",
            "path": "/PlasmaShell",
            "iface": "org.kde.PlasmaShell",
            "member": "wallpaper",
            "signature": "(u)",
            "arguments": [
                new DBus.uint32(0)
            ],
        })
        pendingReply.finished.connect(() => {
            console.log("Current wallpaper is", pendingReply.value["wallpaperPlugin"])
            pendingReply.destroy()
        })
        \endqml

        \note You need to destroy the pending reply object manually in QML
        if you connect to the \c finished signal.
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
                "service": "org.kde.plasmashell",
                "path": "/PlasmaShell",
                "iface": "org.kde.PlasmaShell",
                "member": "wallpaper",
                "signature": "(u)",
                "arguments": [
                    new DBus.uint32(0)
                ],
            }, resolve, reject)
        }).then((result) => {
            console.log("Current wallpaper is", result.value["wallpaperPlugin"])
        }).catch((result) => {
            console.warn("Failed to get the current wallpaper:", result.error.message)
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
        // Get the current wallpaper
        const pendingReply = SessionBus.asyncCall({
            "service": "org.kde.plasmashell",
            "path": "/PlasmaShell",
            "iface": "org.kde.PlasmaShell",
            "member": "wallpaper",
            "signature": "(u)",
            "arguments": [
                new DBus.uint32(0)
            ],
        })
        pendingReply.finished.connect(() => {
            console.log("Current wallpaper is", pendingReply.value["wallpaperPlugin"])
            pendingReply.destroy()
        })
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
        // Enforce a power profile
        const pendingReply = SystemBus.asyncCall({
            "service": "org.freedesktop.UPower.PowerProfiles",
            "path": "/org/freedesktop/UPower/PowerProfiles",
            "iface": "org.freedesktop.UPower.PowerProfiles",
            "member": "HoldProfile",
            "signature": "(sss)",
            "arguments": [
                "performance",
                "A game is running",
                "org.foo.bar",
            ],
        })
        pendingReply.finished.connect(() => {
            console.log("Cookie:", pendingReply.value)
            pendingReply.destroy()
        })
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
