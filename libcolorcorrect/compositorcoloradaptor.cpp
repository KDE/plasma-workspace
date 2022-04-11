/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "compositorcoloradaptor.h"

#include <KLocalizedString>

#include <QDBusInterface>
#include <QDBusReply>

namespace ColorCorrect
{
CompositorAdaptor::CompositorAdaptor(QObject *parent)
    : QObject(parent)
{
    m_iface = new QDBusInterface(QStringLiteral("org.kde.KWin"),
                                 QStringLiteral("/ColorCorrect"),
                                 QStringLiteral("org.kde.kwin.ColorCorrect"),
                                 QDBusConnection::sessionBus(),
                                 this);

    const bool connected = m_iface->connection().connect(QStringLiteral("org.kde.KWin"),
                                                         QStringLiteral("/ColorCorrect"),
                                                         QStringLiteral("org.freedesktop.DBus.Properties"),
                                                         QStringLiteral("PropertiesChanged"),
                                                         this,
                                                         SLOT(handlePropertiesChanged(QString, QVariantMap, QStringList)));
    if (!connected) {
        setError(ErrorCode::ErrorCodeConnectionFailed);
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                          QStringLiteral("/ColorCorrect"),
                                                          QStringLiteral("org.freedesktop.DBus.Properties"),
                                                          QStringLiteral("GetAll"));
    message.setArguments({"org.kde.kwin.ColorCorrect"});

    QDBusPendingReply<QVariantMap> properties = m_iface->connection().asyncCall(message);
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

void CompositorAdaptor::setError(ErrorCode error)
{
    if (m_error == error) {
        return;
    }
    m_error = error;
    switch (error) {
    case ErrorCode::ErrorCodeConnectionFailed:
        m_errorText = i18nc("Critical error message", "Failed to connect to the Window Manager");
        break;
    case ErrorCode::ErrorCodeBackendNoSupport:
        m_errorText = i18nc("Critical error message", "Rendering backend doesn't support Color Correction.");
        break;
    default:
        m_errorText = "";
    }
    Q_EMIT errorChanged();
    Q_EMIT errorTextChanged();
}

void CompositorAdaptor::updateProperties(const QVariantMap &properties)
{
    const QVariant available = properties.value(QStringLiteral("available"));
    if (available.isValid()) {
        m_nightColorAvailable = available.toBool();

        if (!m_nightColorAvailable) {
            setError(ErrorCode::ErrorCodeBackendNoSupport);
            return;
        }
    }

    const QVariant running = properties.value(QStringLiteral("running"));
    if (running.isValid()) {
        if (m_running != running.toBool()) {
            m_running = running.toBool();
            Q_EMIT runningChanged();
        }
    }
}

void CompositorAdaptor::handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    updateProperties(changedProperties);
}

void CompositorAdaptor::sendAutoLocationUpdate(double latitude, double longitude)
{
    m_iface->call(QStringLiteral("nightColorAutoLocationUpdate"), latitude, longitude);
}

void CompositorAdaptor::preview(int temperature)
{
    m_iface->call("preview", (uint)temperature);
}

void CompositorAdaptor::stopPreview()
{
    m_iface->call("stopPreview");
}
}
