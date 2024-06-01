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
inline constexpr QLatin1String KAMELEON_SERVICE("org.kde.kded6");
inline constexpr QLatin1String KAMELEON_PATH("/modules/kameleon");
inline constexpr QLatin1String KAMELEON_INTERFACE("org.kde.kameleon");
}

KeyboardColorControl::KeyboardColorControl(QObject *parent)
    : QObject(parent)
{
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(KAMELEON_SERVICE)) {
        qWarning() << "error connecting to kameleon via dbus: kded service is not registered";
        return;
    }

    QDBusReply<bool> supported =
        QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, u"isSupported"_s));
    if (!supported.isValid()) {
        qWarning() << "error connecting to kameleon via dbus:" << supported.error().message();
        return;
    } else {
        m_supported = supported.value();
        qDebug() << "kameleon supported" << m_supported;
    }

    QDBusReply<bool> enabled =
        QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, u"isEnabled"_s));
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

bool KeyboardColorControl::isSupported() const
{
    return m_supported;
}

bool KeyboardColorControl::enabled() const
{
    return m_enabled.value();
}

void KeyboardColorControl::setEnabled(bool enabled)
{
    if (m_enabled.value() == enabled) {
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, u"setEnabled"_s);
    msg << enabled;
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
    auto watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, wasEnabled = m_enabled.value()](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (QDBusReply<void> reply = *watcher; !reply.isValid()) {
            m_enabled = wasEnabled;
        }
    });

    m_enabled = enabled;
}

#include "moc_keyboardcolorcontrol.cpp"
