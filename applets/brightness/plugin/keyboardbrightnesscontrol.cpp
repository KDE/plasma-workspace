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

using namespace Qt::StringLiterals;

namespace
{
constexpr QLatin1String SOLID_POWERMANAGEMENT_SERVICE("org.kde.Solid.PowerManagement");
}

KeyboardBrightnessControl::KeyboardBrightnessControl(QObject *parent)
    : QObject(parent)
{
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        return;
    }

    if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                               u"/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"_s,
                                               u"org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"_s,
                                               u"keyboardBrightnessChanged"_s,
                                               this,
                                               SLOT(onBrightnessChanged(int)))) {
        qDebug() << "error connecting to Keyboard Brightness changes via dbus";
        return;
    }

    if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                               u"/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"_s,
                                               u"org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"_s,
                                               u"keyboardBrightnessMaxChanged"_s,
                                               this,
                                               SLOT(onBrightnessMaxChanged(int)))) {
        qDebug() << "error connecting to max keyboard Brightness changes via dbus";
        return;
    }

    QDBusMessage brightness = QDBusMessage::createMethodCall(u"org.kde.Solid.PowerManagement"_s,
                                                             u"/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"_s,
                                                             u"org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"_s,
                                                             u"keyboardBrightness"_s);
    QDBusReply<int> brightnessReply = QDBusConnection::sessionBus().call(brightness);
    if (!brightnessReply.isValid()) {
        qDebug() << "error getting keyboard brightness via dbus";
        return;
    }
    m_brightness = brightnessReply.value();

    QDBusMessage brightnessMax = QDBusMessage::createMethodCall(u"org.kde.Solid.PowerManagement"_s,
                                                                u"/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"_s,
                                                                u"org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"_s,
                                                                u"keyboardBrightnessMax"_s);
    QDBusReply<int> brightnessMaxReply = QDBusConnection::sessionBus().call(brightnessMax);
    if (!brightnessReply.isValid()) {
        qDebug() << "error getting max keyboard brightness via dbus";
        return;
    }
    m_maxBrightness = brightnessMaxReply.value();

    m_isBrightnessAvailable = true;
}

KeyboardBrightnessControl::~KeyboardBrightnessControl()
{
}

void KeyboardBrightnessControl::setBrightness(int value)
{
    m_brightness = value;

    QDBusMessage msg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                      u"/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"_s,
                                                      u"org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"_s,
                                                      m_isSilent ? u"setKeyboardBrightnessSilent"_s : u"setKeyboardBrightness"_s);
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

#include "moc_keyboardbrightnesscontrol.cpp"
