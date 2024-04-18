/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nightlightcontrol.h"
#include "nightlightinhibitor.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

static const QString s_serviceName = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_nightLightPath = QStringLiteral("/org/kde/KWin/NightLight");
static const QString s_nightLightInterface = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_propertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");

NightLightControl::NightLightControl(QObject *parent)
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

    m_isInhibitedFromApplet = NightLightInhibitor::instance().isInhibited();
    connect(&NightLightInhibitor::instance(), &NightLightInhibitor::inhibitedChanged, this, [this]() {
        setInhibitedFromApplet(NightLightInhibitor::instance().isInhibited());
    });
}

NightLightControl::~NightLightControl()
{
}

void NightLightControl::handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    updateProperties(changedProperties);
}

void NightLightControl::updateProperties(const QVariantMap &properties)
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

    const QVariant inhibited = properties.value(QStringLiteral("inhibited"));
    if (inhibited.isValid()) {
        setInhibited(inhibited.toBool());
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

bool NightLightControl::isAvailable() const
{
    return m_isAvailable;
}

void NightLightControl::setAvailable(bool available)
{
    if (m_isAvailable == available) {
        return;
    }
    m_isAvailable = available;
    Q_EMIT availableChanged();
}

bool NightLightControl::isEnabled() const
{
    return m_isEnabled;
}

void NightLightControl::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled) {
        return;
    }
    m_isEnabled = enabled;
    Q_EMIT enabledChanged();
}

bool NightLightControl::isRunning() const
{
    return m_isRunning;
}

void NightLightControl::setRunning(bool running)
{
    if (m_isRunning == running) {
        return;
    }
    m_isRunning = running;
    Q_EMIT runningChanged();
}

bool NightLightControl::isInhibited() const
{
    return m_isInhibited;
}

void NightLightControl::setInhibited(bool inhibited)
{
    m_isInhibited = inhibited;
    Q_EMIT inhibitedChanged();
}

void NightLightControl::toggleInhibition()
{
    NightLightInhibitor::instance().toggleInhibition();
}

bool NightLightControl::isInhibitedFromApplet() const
{
    return m_isInhibitedFromApplet;
}

void NightLightControl::setInhibitedFromApplet(bool inhibitedFromApplet)
{
    if (m_isInhibitedFromApplet == inhibitedFromApplet) {
        return;
    }
    m_isInhibitedFromApplet = inhibitedFromApplet;
    Q_EMIT inhibitedFromAppletChanged();
}

int NightLightControl::mode() const
{
    return m_mode;
}

void NightLightControl::setMode(int mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    Q_EMIT modeChanged();
}

bool NightLightControl::isDaylight() const
{
    return m_isDaylight;
}

void NightLightControl::setDaylight(bool daylight)
{
    if (m_isDaylight == daylight) {
        return;
    }
    m_isDaylight = daylight;
    Q_EMIT daylightChanged();
}

int NightLightControl::currentTemperature() const
{
    return m_currentTemperature;
}

void NightLightControl::setCurrentTemperature(int temperature)
{
    if (m_currentTemperature == temperature) {
        return;
    }
    m_currentTemperature = temperature;
    Q_EMIT currentTemperatureChanged();
}

int NightLightControl::targetTemperature() const
{
    return m_targetTemperature;
}

void NightLightControl::setTargetTemperature(int temperature)
{
    if (m_targetTemperature == temperature) {
        return;
    }
    m_targetTemperature = temperature;
    Q_EMIT targetTemperatureChanged();
}

quint64 NightLightControl::currentTransitionEndTime() const
{
    return m_currentTransitionEndTime;
}

void NightLightControl::setCurrentTransitionEndTime(quint64 currentTransitionEndTime)
{
    if (m_currentTransitionEndTime == currentTransitionEndTime) {
        return;
    }
    m_currentTransitionEndTime = currentTransitionEndTime;
    Q_EMIT currentTransitionEndTimeChanged();
}

quint64 NightLightControl::scheduledTransitionStartTime() const
{
    return m_scheduledTransitionStartTime;
}

void NightLightControl::setScheduledTransitionStartTime(quint64 scheduledTransitionStartTime)
{
    if (m_scheduledTransitionStartTime == scheduledTransitionStartTime) {
        return;
    }
    m_scheduledTransitionStartTime = scheduledTransitionStartTime;
    Q_EMIT scheduledTransitionStartTimeChanged();
}

#include "moc_nightlightcontrol.cpp"
