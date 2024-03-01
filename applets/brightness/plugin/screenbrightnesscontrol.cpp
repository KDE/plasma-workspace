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

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";

ScreenBrightnessControl::ScreenBrightnessControl(QObject *parent)
    : QObject(parent)
    , m_isSilent(false)
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        QDBusMessage brightness = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                 QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                                 QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                                 "brightness");

        QDBusReply<int> brightnessReply = QDBusConnection::sessionBus().call(brightness);
        if (!brightnessReply.isValid()) {
            qDebug() << "error getting screen brightness via dbus";
            return;
        }
        m_brightness = brightnessReply.value();

        QDBusMessage brightnessMax = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                    QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                                    QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                                    "brightnessMax");
        QDBusReply<int> brightnessMaxReply = QDBusConnection::sessionBus().call(brightnessMax);
        if (!brightnessReply.isValid()) {
            qDebug() << "error getting max screen brightness via dbus";
            return;
        }
        m_maxBrightness = brightnessMaxReply.value();

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                   QStringLiteral("brightnessChanged"),
                                                   this,
                                                   SLOT(onBrightnessChanged(int)))) {
            qDebug() << "error connecting to Brightness changes via dbus";
            return;
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                   QStringLiteral("maxBrightnessChanged"),
                                                   this,
                                                   SLOT(onBrightnessMaxChanged(int)))) {
            qDebug() << "error connecting to max brightness changes via dbus";
            return;
        }
        m_isBrightnessAvailable = true;
    }
}

ScreenBrightnessControl::~ScreenBrightnessControl()
{
}

void ScreenBrightnessControl::setBrightness(int value)
{
    m_brightness = value;

    QDBusMessage msg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                      QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                      QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                      m_isSilent ? "setBrightnessSilent" : "setBrightness");
    msg << value;
    QDBusConnection::sessionBus().asyncCall(msg);
}

void ScreenBrightnessControl::setIsSilent(bool status)
{
    m_isSilent = status;
}

void ScreenBrightnessControl::onBrightnessChanged(int value)
{
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
