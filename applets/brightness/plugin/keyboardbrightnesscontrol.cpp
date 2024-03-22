/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "keyboardbrightnesscontrol.h"

#include <QCoroDBusPendingCall>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusReply>
#include <QPointer>

using namespace Qt::StringLiterals;

namespace
{
constexpr QLatin1String SOLID_POWERMANAGEMENT_SERVICE("org.kde.Solid.PowerManagement");
}

KeyboardBrightnessControl::KeyboardBrightnessControl(QObject *parent)
    : QObject(parent)
{
    init();
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

QCoro::Task<void> KeyboardBrightnessControl::init()
{
    QDBusMessage brightnessMax = QDBusMessage::createMethodCall(u"org.kde.Solid.PowerManagement"_s,
                                                                u"/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"_s,
                                                                u"org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"_s,
                                                                u"keyboardBrightnessMax"_s);
    QPointer<KeyboardBrightnessControl> alive{this};
    const QDBusReply<int> brightnessMaxReply = co_await QDBusConnection::sessionBus().asyncCall(brightnessMax);
    if (!alive || !brightnessMaxReply.isValid()) {
        qDebug() << "error getting max keyboard brightness via dbus" << brightnessMaxReply.error();
        co_return;
    }
    m_maxBrightness = brightnessMaxReply.value();

    QDBusMessage brightness = QDBusMessage::createMethodCall(u"org.kde.Solid.PowerManagement"_s,
                                                             u"/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"_s,
                                                             u"org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"_s,
                                                             u"keyboardBrightness"_s);
    const QDBusReply<int> brightnessReply = co_await QDBusConnection::sessionBus().asyncCall(brightness);
    if (!alive || !brightnessReply.isValid()) {
        qDebug() << "error getting keyboard brightness via dbus" << brightnessReply.error();
        co_return;
    }
    m_brightness = brightnessReply.value();

    if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                               u"/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"_s,
                                               u"org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"_s,
                                               u"keyboardBrightnessChanged"_s,
                                               this,
                                               SLOT(onBrightnessChanged(int)))) {
        qDebug() << "error connecting to Keyboard Brightness changes via dbus";
        co_return;
    }

    if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                               u"/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"_s,
                                               u"org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"_s,
                                               u"keyboardBrightnessMaxChanged"_s,
                                               this,
                                               SLOT(onBrightnessMaxChanged(int)))) {
        qDebug() << "error connecting to max keyboard Brightness changes via dbus";
        co_return;
    }

    m_isBrightnessAvailable = true;
}

#include "moc_keyboardbrightnesscontrol.cpp"
