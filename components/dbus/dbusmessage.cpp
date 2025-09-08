/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbusmessage.h"

#include <QDBusMessage>

using namespace Qt::StringLiterals;

namespace Plasma
{

DBusMessage::DBusMessage() = default;

DBusMessage::DBusMessage(const QDBusMessage &message)
    : m_service(message.service())
    , m_path(message.path())
    , m_interface(message.interface())
    , m_member(message.member())
    , m_arguments(message.arguments())
    , m_signature(message.signature())
{
}

DBusMessage::DBusMessage(const QVariantMap &data)
    : m_service(data.value(u"service"_s).toString())
    , m_path(data.value(u"path"_s).toString())
    , m_interface(data.value(u"interface"_s).toString())
    , m_member(data.value(u"member"_s).toString())
    , m_arguments(data.value(u"arguments"_s).toList())
    , m_signature(data.value(u"signature"_s).toString())
{
}

QString DBusMessage::service() const
{
    return m_service;
}

void DBusMessage::setService(const QString &value)
{
    m_service = value;
}

QString DBusMessage::path() const
{
    return m_path;
}

void DBusMessage::setPath(const QString &path)
{
    m_path = path;
}

QString DBusMessage::interface() const
{
    return m_interface;
}

void DBusMessage::setInterface(const QString &iface)
{
    m_interface = iface;
}

QString DBusMessage::member() const
{
    return m_member;
}

void DBusMessage::setMember(const QString &name)
{
    m_member = name;
}

QVariantList DBusMessage::arguments() const
{
    return m_arguments;
}

void DBusMessage::setArguments(const QVariantList &args)
{
    m_arguments = args;
}

QString DBusMessage::signature() const
{
    return m_signature;
}

void DBusMessage::setSignature(const QString &sig)
{
    m_signature = sig;
}
}

#include "moc_dbusmessage.cpp"
