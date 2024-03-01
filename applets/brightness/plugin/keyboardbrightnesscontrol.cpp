/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "keyboardbrightnesscontrol.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusReply>

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";

KeyboardBrightnessControl::KeyboardBrightnessControl(QObject *parent)
    : QObject(parent)
    , m_brightness(0)
    , m_maxBrightness(0)
    , m_isSilent(false)
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        QDBusMessage brightness = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                 QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                                 QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                                 "keyboardBrightness");

        QDBusReply<int> brightnessReply = QDBusConnection::sessionBus().call(brightness);
        if (!brightnessReply.isValid()) {
            qDebug() << "error getting keyboard brightness via dbus";
            return;
        }
        m_brightness = brightnessReply.value();

        QDBusMessage brightnessMax = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                    QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                                    QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                                    "keyboardBrightnessMax");
        QDBusReply<int> brightnessMaxReply = QDBusConnection::sessionBus().call(brightnessMax);
        if (!brightnessReply.isValid()) {
            qDebug() << "error getting max keyboard brightness via dbus";
            return;
        }
        m_maxBrightness = brightnessMaxReply.value();

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                   QStringLiteral("keyboardBrightnessChanged"),
                                                   this,
                                                   SLOT(onBrightnessChanged(int)))) {
            qDebug() << "error connecting to Keyboard Brightness changes via dbus";
            return;
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                   QStringLiteral("keyboardBrightnessMaxChanged"),
                                                   this,
                                                   SLOT(onBrightnessMaxChanged(int)))) {
            qDebug() << "error connecting to max keyboard Brightness changes via dbus";
            return;
        }
        m_isBrightnessAvailable = true;
    }
}

KeyboardBrightnessControl::~KeyboardBrightnessControl()
{
}

void KeyboardBrightnessControl::setBrightness(int value)
{
    m_brightness = value;

    QDBusMessage msg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                      QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                      QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                      m_isSilent ? "setKeyboardBrightnessSilent" : "setKeyboardBrightness");
    msg << value;
    QDBusConnection::sessionBus().asyncCall(msg);
}

QBindable<bool> KeyboardBrightnessControl::bindableIsBrightnessAvailable()
{
    return &m_isBrightnessAvailable;
}

QBindable<int> KeyboardBrightnessControl::bindableBrightness()
{
    return &m_brightness;
}

QBindable<int> KeyboardBrightnessControl::bindableBrightnessMax()
{
    return &m_maxBrightness;
}

void KeyboardBrightnessControl::setIsSilent(bool silent)
{
    m_isSilent = silent;
}

void KeyboardBrightnessControl::onBrightnessChanged(int value)
{
    m_brightness = value;
}

void KeyboardBrightnessControl::onBrightnessMaxChanged(int value)
{
    m_maxBrightness = value;

    m_isBrightnessAvailable = value > 0;
}
