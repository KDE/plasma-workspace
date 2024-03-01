/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "screenbrightnesscontrol.h"

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

ScreenBrightnessControl::ScreenBrightnessControl(QObject *parent)
    : QObject(parent)
{
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        return;
    }

    if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                               u"/org/kde/Solid/PowerManagement/Actions/BrightnessControl"_s,
                                               u"org.kde.Solid.PowerManagement.Actions.BrightnessControl"_s,
                                               u"brightnessChanged"_s,
                                               this,
                                               SLOT(onBrightnessChanged(int)))) {
        qDebug() << "error connecting to Brightness changes via dbus";
        return;
    }

    if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                               u"/org/kde/Solid/PowerManagement/Actions/BrightnessControl"_s,
                                               u"org.kde.Solid.PowerManagement.Actions.BrightnessControl"_s,
                                               u"maxBrightnessChanged"_s,
                                               this,
                                               SLOT(onBrightnessMaxChanged(int)))) {
        qDebug() << "error connecting to max brightness changes via dbus";
        return;
    }

    QDBusMessage brightnessMax = QDBusMessage::createMethodCall(u"org.kde.Solid.PowerManagement"_s,
                                                                u"/org/kde/Solid/PowerManagement/Actions/BrightnessControl"_s,
                                                                u"org.kde.Solid.PowerManagement.Actions.BrightnessControl"_s,
                                                                u"brightnessMax"_s);
    QDBusReply<int> brightnessMaxReply = QDBusConnection::sessionBus().call(brightnessMax);
    if (!brightnessMaxReply.isValid()) {
        qDebug() << "error getting max screen brightness via dbus";
        return;
    }
    m_maxBrightness = brightnessMaxReply.value();

    QDBusMessage brightness = QDBusMessage::createMethodCall(u"org.kde.Solid.PowerManagement"_s,
                                                             u"/org/kde/Solid/PowerManagement/Actions/BrightnessControl"_s,
                                                             u"org.kde.Solid.PowerManagement.Actions.BrightnessControl"_s,
                                                             u"brightness"_s);

    QDBusReply<int> brightnessReply = QDBusConnection::sessionBus().call(brightness);
    if (!brightnessReply.isValid()) {
        qDebug() << "error getting screen brightness via dbus";
        return;
    }
    m_brightness = brightnessReply.value();

    m_isBrightnessAvailable = true;
}

ScreenBrightnessControl::~ScreenBrightnessControl()
{
}

void ScreenBrightnessControl::setBrightness(int value)
{
    if (m_brightness == value) {
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                      u"/org/kde/Solid/PowerManagement/Actions/BrightnessControl"_s,
                                                      u"org.kde.Solid.PowerManagement.Actions.BrightnessControl"_s,
                                                      m_isSilent ? u"setBrightnessSilent"_s : u"setBrightness"_s);
    msg << value;
    QDBusPendingCall async = QDBusConnection::sessionBus().asyncCall(msg);
    m_brightnessChangeWatcher.reset(new QDBusPendingCallWatcher(async));
    connect(m_brightnessChangeWatcher.get(),
            &QDBusPendingCallWatcher::finished,
            this,
            [this, oldValue = m_brightness.value()](QDBusPendingCallWatcher *watcher) {
                const QDBusPendingReply<void> reply = *watcher;
                if (reply.isError()) {
                    qDebug() << "error setting brightness via dbus" << reply.error();
                    m_brightness = oldValue;
                }
                m_brightnessChangeWatcher.reset();
            });

    m_brightness = value;
}

void ScreenBrightnessControl::setIsSilent(bool status)
{
    m_isSilent = status;
}

void ScreenBrightnessControl::onBrightnessChanged(int value)
{
    if (m_brightnessChangeWatcher) {
        return;
    }
    m_brightness = value;
}

void ScreenBrightnessControl::onBrightnessMaxChanged(int value)
{
    m_maxBrightness = value;

    m_isBrightnessAvailable = value > 0;
}

QBindable<bool> ScreenBrightnessControl::bindableIsBrightnessAvailable()
{
    return &m_isBrightnessAvailable;
}

QBindable<int> ScreenBrightnessControl::bindableBrightness()
{
    return &m_brightness;
}

QBindable<int> ScreenBrightnessControl::bindableBrightnessMax()
{
    return &m_maxBrightness;
}

#include "moc_screenbrightnesscontrol.cpp"
