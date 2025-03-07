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

namespace Plasma
{
class DBusConnection;
class DBusMessage;

/*!
    \qmltype DBusPendingReply
    \inherits QtObject
    \inmodule org.kde.plasma.workspace.dbus
    \brief The DBusPendingReply class represents the result of an asynchronous D-Bus method call.

    DBusPendingReply is used to handle the outcome of an asynchronous D-Bus method call. It allows
    developers to check if the call has finished, if it has an error, and retrieve the result
    data. When an asynchronous D-Bus method call is made, a DBusPendingReply object is returned,
    which can be used to track the progress and get the final result of the call.

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

    \sa DBusConnection::asyncCall
*/
class DBusPendingReply : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_DISABLE_COPY_MOVE(DBusPendingReply)

    /*!
        \qmlproperty bool DBusPendingReply::isFinished

        Whether the asynchronous D-Bus method call has finished.
        If the call is completed, either successfully or with an error, this property will be true.
     */
    Q_PROPERTY(bool isFinished READ isFinished NOTIFY finished)

    /*!
        \qmlproperty bool DBusPendingReply::isError

        Whether the asynchronous D-Bus method call has encountered an error.
        If there is an error during the call, this property will be true.
     */
    Q_PROPERTY(bool isError READ isError NOTIFY finished)

    /*!
        \qmlproperty bool DBusPendingReply::isValid

        Whether the D-Bus reply is valid.
        A valid reply means the D-Bus method call has finished without errors and the result data can be retrieved.
     */
    Q_PROPERTY(bool isValid READ isValid NOTIFY finished)

    /*!
        \qmlproperty DBusError DBusPendingReply::error

        The error information if the asynchronous D-Bus method call has failed.
        It provides details about the error that occurred during the call.
     */
    Q_PROPERTY(DBusError error READ error NOTIFY finished)

    /*!
        \qmlproperty var DBusPendingReply::value

        The first value of the result data from the asynchronous D-Bus method call.
        If the call is successful and there are result arguments, this property returns the first argument.
        If there are no arguments, it returns null.
     */
    Q_PROPERTY(QVariant value READ value NOTIFY finished)

    /*!
        \qmlproperty list<var> DBusPendingReply::values

        The list of result values from the asynchronous D-Bus method call.
        If the call is successful, this property returns all the result arguments as a list.
        If there are no arguments, it returns an empty list.
     */
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
    /*!
        \qmlsignal org.kde.plasma.workspace.dbus::DBusPendingReply::finished

        This signal is emitted when the asynchronous D-Bus method call has finished.
        It indicates that the call is completed, either successfully or with an error.

        \note You need to destroy the pending reply object manually in QML
        if you connect to the \c finished signal.
     */
    void finished();

private:
    void callInternal(const QDBusConnection &connection, const DBusMessage &message, const QByteArray &signature = {});

    bool m_isFinished = false;
    QDBusMessage m_reply;
    QVariantList m_arguments;
};
}
