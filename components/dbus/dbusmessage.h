/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QVariantList>
#include <qqmlregistration.h>

class QDBusMessage;

namespace Plasma
{
class DBusMessage
{
    Q_GADGET
    QML_VALUE_TYPE(dbusMessage)
    QML_STRUCTURED_VALUE

    /**
     * The bus address of the method call
     */
    Q_PROPERTY(QString service READ service WRITE setService)

    /**
     * The object path of the method call is being sent to
     */
    Q_PROPERTY(QString path READ path WRITE setPath)

    /**
     * The interface of the method call
     */
    Q_PROPERTY(QString iface READ interface WRITE setInterface)

    /**
     * The name of the method in the method call
     */
    Q_PROPERTY(QString member READ member WRITE setMember)

    /**
     * The list of arguments that are going to be sent
     */
    Q_PROPERTY(QVariantList arguments READ arguments WRITE setArguments)

    /**
     * The signature for the output arguments of a method call.
     */
    Q_PROPERTY(QString signature READ signature WRITE setSignature)

public:
    explicit DBusMessage();
    Q_INVOKABLE explicit DBusMessage(const QVariantMap &data);
    explicit DBusMessage(const QDBusMessage &message);

    QString service() const;
    void setService(const QString &value);

    QString path() const;
    void setPath(const QString &path);

    QString interface() const;
    void setInterface(const QString &iface);

    QString member() const;
    void setMember(const QString &name);

    QVariantList arguments() const;
    void setArguments(const QVariantList &args);

    QString signature() const;
    void setSignature(const QString &sig);

private:
    QString m_service;
    QString m_path;
    QString m_interface;
    QString m_member;
    QVariantList m_arguments;
    QString m_signature;
};
}
