/*
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "keyboardcolorcontrol.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusReply>
#include <qdbusmessage.h>

using namespace Qt::StringLiterals;

namespace
{
constexpr QLatin1String KAMELEON_SERVICE("org.kde.kded6");
constexpr QLatin1String KAMELEON_PATH("/modules/kameleon");
constexpr QLatin1String KAMELEON_INTERFACE("org.kde.kameleon");
}

KeyboardColorControl::KeyboardColorControl(QObject *parent)
    : QObject(parent)
{
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(KAMELEON_SERVICE)) {
        qWarning() << "error connecting to kameleon via dbus: kded service is not registered";
        return;
    }

    QDBusReply<bool> supported =
        QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, "isSupported"));
    if (!supported.isValid()) {
        qWarning() << "error connecting to kameleon via dbus:" << supported.error().message();
        return;
    } else {
        m_supported = supported.value();
        qDebug() << "kameleon supported" << m_supported;
    }

    QDBusReply<bool> enabled =
        QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, "isEnabled"));
    if (!enabled.isValid()) {
        qWarning() << "error connecting to kameleon via dbus:" << enabled.error().message();
        return;
    } else {
        m_enabled = enabled.value();
        qDebug() << "kameleon enabled" << m_enabled;
    }
}

KeyboardColorControl::~KeyboardColorControl()
{
}

bool KeyboardColorControl::isSupported()
{
    return m_supported;
}

bool KeyboardColorControl::isEnabled()
{
    return m_enabled;
}

void KeyboardColorControl::setEnabled(bool enabled)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, "setEnabled");
    msg << enabled;
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "error connecting to kameleon via dbus:" << reply.errorMessage();
        return;
    }
    m_enabled = enabled;
}

#include "moc_keyboardcolorcontrol.cpp"
