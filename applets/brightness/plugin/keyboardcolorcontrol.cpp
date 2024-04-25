/*
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "keyboardcolorcontrol.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusReply>

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
        qWarning() << "error connecting to kameleon isSupported via dbus:" << supported.error().message();
        return;
    } else {
        m_supported = supported.value();
    }

    QDBusReply<bool> accent =
        QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, "isAccent"));
    if (!accent.isValid()) {
        qWarning() << "error connecting to kameleon isAccent via dbus:" << accent.error().message();
        m_supported = false;
        return;
    } else {
        m_accent = accent.value();
    }

    QDBusReply<QString> currentColor =
        QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, "currentColor"));
    if (!currentColor.isValid()) {
        qWarning() << "error connecting to kameleon currentColor via dbus:" << currentColor.error().message();
        m_supported = false;
        return;
    } else {
        m_color = currentColor.value();
        qDebug() << "kameleon current color" << m_color;
    }

    if (!QDBusConnection::sessionBus().connect(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, "accentChanged", this, SLOT(slotAccentChanged(bool)))) {
        qWarning() << "error connecting to kameleon accentChanged via dbus";
        m_supported = false;
        return;
    }

    if (!QDBusConnection::sessionBus()
             .connect(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, "activeColorChanged", this, SLOT(slotColorChanged(QString)))) {
        qWarning() << "error connecting to kameleon activeColorChanged via dbus";
        m_supported = false;
        return;
    }
}

KeyboardColorControl::~KeyboardColorControl()
{
}

bool KeyboardColorControl::isSupported()
{
    return m_supported;
}

bool KeyboardColorControl::isAccent()
{
    return m_accent;
}

QString KeyboardColorControl::color()
{
    return m_color;
}

void KeyboardColorControl::setAccent(bool accent)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, "setAccent");
    msg << accent;
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "error connecting to kameleon setaAccent via dbus:" << reply.errorMessage();
        return;
    }
    m_accent = accent;
}

void KeyboardColorControl::setColor(const QString &color)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(KAMELEON_SERVICE, KAMELEON_PATH, KAMELEON_INTERFACE, "setColor");
    msg << color;
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "error connecting to kameleon setColor via dbus:" << reply.errorMessage();
        return;
    }
    m_color = color;
}

void KeyboardColorControl::slotAccentChanged(bool accent)
{
    qDebug() << "kameleon accent changed" << accent;
    m_accent = accent;
    Q_EMIT accentChanged();
}

void KeyboardColorControl::slotColorChanged(QString color)
{
    qDebug() << "kameleon active color changed" << color;
    m_color = color;
    Q_EMIT colorChanged();
}

#include "moc_keyboardcolorcontrol.cpp"
