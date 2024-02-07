/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nightcolormonitor.h"
#include "nightcolormonitor_p.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <qstringliteral.h>
#include <qtypes.h>
#include <qvariant.h>

static const QString s_serviceName = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_nightColorPath = QStringLiteral("/org/kde/KWin/NightLight");
static const QString s_nightColorInterface = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_propertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");

NightColorMonitorPrivate::NightColorMonitorPrivate(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    // clang-format off
    const bool connected = bus.connect(s_serviceName,
                                       s_nightColorPath,
                                       s_propertiesInterface,
                                       QStringLiteral("PropertiesChanged"),
                                       this,
                                       SLOT(handlePropertiesChanged(QString,QVariantMap,QStringList)));
    // clang-format on
    if (!connected) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, s_nightColorPath, s_propertiesInterface, QStringLiteral("GetAll"));
    message.setArguments({s_nightColorInterface});

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

NightColorMonitorPrivate::~NightColorMonitorPrivate()
{
}

void NightColorMonitorPrivate::handlePropertiesChanged(const QString &interfaceName,
                                                       const QVariantMap &changedProperties,
                                                       const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    updateProperties(changedProperties);
}

int NightColorMonitorPrivate::currentTemperature() const
{
    return m_currentTemperature;
}

int NightColorMonitorPrivate::targetTemperature() const
{
    return m_targetTemperature;
}

void NightColorMonitorPrivate::updateProperties(const QVariantMap &properties)
{
    const QVariant available = properties.value(QStringLiteral("available"));
    if (available.isValid()) {
        setAvailable(available.toBool());
    }

    const QVariant enabled = properties.value(QStringLiteral("enabled"));
    if (enabled.isValid()) {
        setEnabled(enabled.toBool());
    }

    const QVariant running = properties.value(QStringLiteral("running"));
    if (running.isValid()) {
        setRunning(running.toBool());
    }

    const QVariant mode = properties.value(QStringLiteral("mode"));
    if (mode.isValid()) {
        setMode(mode.toInt());
    }

    const QVariant daylight = properties.value(QStringLiteral("daylight"));
    if (daylight.isValid()) {
        setDaylight(daylight.toBool());
    }

    const QVariant currentTemperature = properties.value(QStringLiteral("currentTemperature"));
    if (currentTemperature.isValid()) {
        setCurrentTemperature(currentTemperature.toInt());
    }

    const QVariant targetTemperature = properties.value(QStringLiteral("targetTemperature"));
    if (targetTemperature.isValid()) {
        setTargetTemperature(targetTemperature.toInt());
    }

    const QVariant currentTransitionStartTime = properties.value(QStringLiteral("previousTransitionDateTime"));
    const QVariant currentTransitionDuration = properties.value(QStringLiteral("previousTransitionDuration"));
    if (currentTransitionStartTime.isValid() && currentTransitionDuration.isValid()) {
        setCurrentTransitionEndTime(currentTransitionStartTime.toULongLong() * 1000 + currentTransitionDuration.toUInt());
    }

    const QVariant scheduledTransitionStartTime = properties.value(QStringLiteral("scheduledTransitionDateTime"));
    if (scheduledTransitionStartTime.isValid()) {
        setScheduledTransitionStartTime(scheduledTransitionStartTime.toULongLong() * 1000);
    }
}

void NightColorMonitorPrivate::setCurrentTemperature(int temperature)
{
    if (m_currentTemperature == temperature) {
        return;
    }
    m_currentTemperature = temperature;
    Q_EMIT currentTemperatureChanged();
}

void NightColorMonitorPrivate::setTargetTemperature(int temperature)
{
    if (m_targetTemperature == temperature) {
        return;
    }
    m_targetTemperature = temperature;
    Q_EMIT targetTemperatureChanged();
}

bool NightColorMonitorPrivate::isAvailable() const
{
    return m_isAvailable;
}

void NightColorMonitorPrivate::setAvailable(bool available)
{
    if (m_isAvailable == available) {
        return;
    }
    m_isAvailable = available;
    Q_EMIT availableChanged();
}

bool NightColorMonitorPrivate::isEnabled() const
{
    return m_isEnabled;
}

void NightColorMonitorPrivate::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled) {
        return;
    }
    m_isEnabled = enabled;
    Q_EMIT enabledChanged();
}

bool NightColorMonitorPrivate::isRunning() const
{
    return m_isRunning;
}

void NightColorMonitorPrivate::setRunning(bool running)
{
    if (m_isRunning == running) {
        return;
    }
    m_isRunning = running;
    Q_EMIT runningChanged();
}

int NightColorMonitorPrivate::mode() const
{
    return m_mode;
}

void NightColorMonitorPrivate::setMode(int mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    Q_EMIT modeChanged();
}

bool NightColorMonitorPrivate::isDaylight() const
{
    return m_isDaylight;
}

void NightColorMonitorPrivate::setDaylight(bool daylight)
{
    if (m_isDaylight == daylight) {
        return;
    }
    m_isDaylight = daylight;
    Q_EMIT daylightChanged();
}

quint64 NightColorMonitorPrivate::currentTransitionEndTime() const
{
    return m_currentTransitionEndTime;
}

void NightColorMonitorPrivate::setCurrentTransitionEndTime(quint64 currentTransitionEndTime)
{
    if (m_currentTransitionEndTime == currentTransitionEndTime) {
        return;
    }
    m_currentTransitionEndTime = currentTransitionEndTime;
    Q_EMIT currentTransitionEndTimeChanged();
}

quint64 NightColorMonitorPrivate::scheduledTransitionStartTime() const
{
    return m_scheduledTransitionStartTime;
}

void NightColorMonitorPrivate::setScheduledTransitionStartTime(quint64 scheduledTransitionStartTime)
{
    if (m_scheduledTransitionStartTime == scheduledTransitionStartTime) {
        return;
    }
    m_scheduledTransitionStartTime = scheduledTransitionStartTime;
    Q_EMIT scheduledTransitionStartTimeChanged();
}

NightColorMonitor::NightColorMonitor(QObject *parent)
    : QObject(parent)
    , d(new NightColorMonitorPrivate(this))
{
    connect(d, &NightColorMonitorPrivate::availableChanged, this, &NightColorMonitor::availableChanged);
    connect(d, &NightColorMonitorPrivate::enabledChanged, this, &NightColorMonitor::enabledChanged);
    connect(d, &NightColorMonitorPrivate::runningChanged, this, &NightColorMonitor::runningChanged);
    connect(d, &NightColorMonitorPrivate::modeChanged, this, &NightColorMonitor::modeChanged);
    connect(d, &NightColorMonitorPrivate::daylightChanged, this, &NightColorMonitor::daylightChanged);
    connect(d, &NightColorMonitorPrivate::currentTransitionEndTimeChanged, this, &NightColorMonitor::currentTransitionEndTimeChanged);
    connect(d, &NightColorMonitorPrivate::scheduledTransitionStartTimeChanged, this, &NightColorMonitor::scheduledTransitionStartTimeChanged);
    connect(d, &NightColorMonitorPrivate::currentTemperatureChanged, this, &NightColorMonitor::currentTemperatureChanged);
    connect(d, &NightColorMonitorPrivate::targetTemperatureChanged, this, &NightColorMonitor::targetTemperatureChanged);
}

NightColorMonitor::~NightColorMonitor()
{
}

bool NightColorMonitor::isAvailable() const
{
    return d->isAvailable();
}

bool NightColorMonitor::isEnabled() const
{
    return d->isEnabled();
}

bool NightColorMonitor::isRunning() const
{
    return d->isRunning();
}

int NightColorMonitor::mode() const
{
    return d->mode();
}

bool NightColorMonitor::isDaylight() const
{
    return d->isDaylight();
}

int NightColorMonitor::currentTemperature() const
{
    return d->currentTemperature();
}

int NightColorMonitor::targetTemperature() const
{
    return d->targetTemperature();
}

quint64 NightColorMonitor::currentTransitionEndTime() const
{
    return d->currentTransitionEndTime();
}

quint64 NightColorMonitor::scheduledTransitionStartTime() const
{
    return d->scheduledTransitionStartTime();
}
