/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nightlightmonitor.h"
#include "nightlightmonitor_p.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

static const QString s_serviceName = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_nightLightPath = QStringLiteral("/org/kde/KWin/NightLight");
static const QString s_nightLightInterface = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_propertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");

NightLightMonitorPrivate::NightLightMonitorPrivate(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    // clang-format off
    const bool connected = bus.connect(s_serviceName,
                                       s_nightLightPath,
                                       s_propertiesInterface,
                                       QStringLiteral("PropertiesChanged"),
                                       this,
                                       SLOT(handlePropertiesChanged(QString,QVariantMap,QStringList)));
    // clang-format on
    if (!connected) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, s_nightLightPath, s_propertiesInterface, QStringLiteral("GetAll"));
    message.setArguments({s_nightLightInterface});

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

NightLightMonitorPrivate::~NightLightMonitorPrivate()
{
}

void NightLightMonitorPrivate::handlePropertiesChanged(const QString &interfaceName,
                                                       const QVariantMap &changedProperties,
                                                       const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    updateProperties(changedProperties);
}

int NightLightMonitorPrivate::currentTemperature() const
{
    return m_currentTemperature;
}

int NightLightMonitorPrivate::targetTemperature() const
{
    return m_targetTemperature;
}

void NightLightMonitorPrivate::updateProperties(const QVariantMap &properties)
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

void NightLightMonitorPrivate::setCurrentTemperature(int temperature)
{
    if (m_currentTemperature == temperature) {
        return;
    }
    m_currentTemperature = temperature;
    Q_EMIT currentTemperatureChanged();
}

void NightLightMonitorPrivate::setTargetTemperature(int temperature)
{
    if (m_targetTemperature == temperature) {
        return;
    }
    m_targetTemperature = temperature;
    Q_EMIT targetTemperatureChanged();
}

bool NightLightMonitorPrivate::isAvailable() const
{
    return m_isAvailable;
}

void NightLightMonitorPrivate::setAvailable(bool available)
{
    if (m_isAvailable == available) {
        return;
    }
    m_isAvailable = available;
    Q_EMIT availableChanged();
}

bool NightLightMonitorPrivate::isEnabled() const
{
    return m_isEnabled;
}

void NightLightMonitorPrivate::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled) {
        return;
    }
    m_isEnabled = enabled;
    Q_EMIT enabledChanged();
}

bool NightLightMonitorPrivate::isRunning() const
{
    return m_isRunning;
}

void NightLightMonitorPrivate::setRunning(bool running)
{
    if (m_isRunning == running) {
        return;
    }
    m_isRunning = running;
    Q_EMIT runningChanged();
}

int NightLightMonitorPrivate::mode() const
{
    return m_mode;
}

void NightLightMonitorPrivate::setMode(int mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    Q_EMIT modeChanged();
}

bool NightLightMonitorPrivate::isDaylight() const
{
    return m_isDaylight;
}

void NightLightMonitorPrivate::setDaylight(bool daylight)
{
    if (m_isDaylight == daylight) {
        return;
    }
    m_isDaylight = daylight;
    Q_EMIT daylightChanged();
}

quint64 NightLightMonitorPrivate::currentTransitionEndTime() const
{
    return m_currentTransitionEndTime;
}

void NightLightMonitorPrivate::setCurrentTransitionEndTime(quint64 currentTransitionEndTime)
{
    if (m_currentTransitionEndTime == currentTransitionEndTime) {
        return;
    }
    m_currentTransitionEndTime = currentTransitionEndTime;
    Q_EMIT currentTransitionEndTimeChanged();
}

quint64 NightLightMonitorPrivate::scheduledTransitionStartTime() const
{
    return m_scheduledTransitionStartTime;
}

void NightLightMonitorPrivate::setScheduledTransitionStartTime(quint64 scheduledTransitionStartTime)
{
    if (m_scheduledTransitionStartTime == scheduledTransitionStartTime) {
        return;
    }
    m_scheduledTransitionStartTime = scheduledTransitionStartTime;
    Q_EMIT scheduledTransitionStartTimeChanged();
}

NightLightMonitor::NightLightMonitor(QObject *parent)
    : QObject(parent)
    , d(new NightLightMonitorPrivate(this))
{
    connect(d, &NightLightMonitorPrivate::availableChanged, this, &NightLightMonitor::availableChanged);
    connect(d, &NightLightMonitorPrivate::enabledChanged, this, &NightLightMonitor::enabledChanged);
    connect(d, &NightLightMonitorPrivate::runningChanged, this, &NightLightMonitor::runningChanged);
    connect(d, &NightLightMonitorPrivate::modeChanged, this, &NightLightMonitor::modeChanged);
    connect(d, &NightLightMonitorPrivate::daylightChanged, this, &NightLightMonitor::daylightChanged);
    connect(d, &NightLightMonitorPrivate::currentTransitionEndTimeChanged, this, &NightLightMonitor::currentTransitionEndTimeChanged);
    connect(d, &NightLightMonitorPrivate::scheduledTransitionStartTimeChanged, this, &NightLightMonitor::scheduledTransitionStartTimeChanged);
    connect(d, &NightLightMonitorPrivate::currentTemperatureChanged, this, &NightLightMonitor::currentTemperatureChanged);
    connect(d, &NightLightMonitorPrivate::targetTemperatureChanged, this, &NightLightMonitor::targetTemperatureChanged);
}

NightLightMonitor::~NightLightMonitor()
{
}

bool NightLightMonitor::isAvailable() const
{
    return d->isAvailable();
}

bool NightLightMonitor::isEnabled() const
{
    return d->isEnabled();
}

bool NightLightMonitor::isRunning() const
{
    return d->isRunning();
}

int NightLightMonitor::mode() const
{
    return d->mode();
}

bool NightLightMonitor::isDaylight() const
{
    return d->isDaylight();
}

int NightLightMonitor::currentTemperature() const
{
    return d->currentTemperature();
}

int NightLightMonitor::targetTemperature() const
{
    return d->targetTemperature();
}

quint64 NightLightMonitor::currentTransitionEndTime() const
{
    return d->currentTransitionEndTime();
}

quint64 NightLightMonitor::scheduledTransitionStartTime() const
{
    return d->scheduledTransitionStartTime();
}

#include "moc_nightlightmonitor.cpp"
