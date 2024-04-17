/*
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "colorblindnesscorrectioncontrol.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

using namespace Qt::StringLiterals;

namespace
{
static const QString s_service = QStringLiteral("org.kde.KWin");
static const QString s_path = QStringLiteral("/Effects");
static const QString s_interface = QStringLiteral("org.kde.kwin.Effects");
static const QString s_propertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
static const QString s_effectName = QStringLiteral("colorblindnesscorrection");
}

ColorBlindnessCorrectionControl::ColorBlindnessCorrectionControl(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    if (!bus.interface()->isServiceRegistered(s_service)) {
        qWarning() << "error connecting to effects via dbus: kwin service is not registered";
        return;
    }
    // clang-format off
    const bool connected = bus.connect(s_service,
                                       s_path,
                                       s_propertiesInterface,
                                       QStringLiteral("PropertiesChanged"),
                                       this,
                                       SLOT(handlePropertiesChanged(QString,QVariantMap,QStringList)));
    // clang-format on
    if (!connected) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_service, s_path, s_propertiesInterface, QStringLiteral("GetAll"));
    message.setArguments({s_interface});

    QDBusPendingReply<QVariantMap> properties = bus.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(properties, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        self->deleteLater();

        const QDBusPendingReply<QVariantMap> properties = *self;
        if (properties.isError()) {
            return;
        }

        updateProperties(properties.value());
    });
}

ColorBlindnessCorrectionControl::~ColorBlindnessCorrectionControl()
{
}

void ColorBlindnessCorrectionControl::handlePropertiesChanged(const QString &interfaceName,
                                                              const QVariantMap &changedProperties,
                                                              const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    updateProperties(changedProperties);
}

void ColorBlindnessCorrectionControl::updateProperties(const QVariantMap &properties)
{
    const QVariant availableEffects = properties.value(QStringLiteral("listOfEffects"));
    if (availableEffects.isValid()) {
        m_supported = availableEffects.toStringList().contains(s_effectName);
        qDebug() << "effect supported" << m_supported;
    }

    const QVariant loadedEffects = properties.value(QStringLiteral("loadedEffects"));
    if (loadedEffects.isValid()) {
        m_enabled = loadedEffects.toStringList().contains(s_effectName);
        qDebug() << "effect enabled" << m_enabled;
    }
}

bool ColorBlindnessCorrectionControl::isSupported()
{
    return m_supported;
}

bool ColorBlindnessCorrectionControl::isShown()
{
    return m_shown;
}

bool ColorBlindnessCorrectionControl::isEnabled()
{
    return m_enabled;
}

void ColorBlindnessCorrectionControl::setEnabled(bool enabled)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(s_service, s_path, s_interface, enabled ? "loadEffect" : "unloadEffect");
    msg << s_effectName;
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "error connecting to effects via dbus:" << reply.errorMessage();
        return;
    }
    m_enabled = enabled;
}

#include "moc_keyboardcolorcontrol.cpp"
