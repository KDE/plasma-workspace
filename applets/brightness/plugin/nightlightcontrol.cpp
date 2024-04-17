/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nightlightcontrol.h"
#include "nightlightcontrol_p.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

static const QString s_serviceName = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_nightLightPath = QStringLiteral("/org/kde/KWin/NightLight");
static const QString s_nightLightInterface = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_propertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");

NightLightControlPrivate::NightLightControlPrivate(QObject *parent)
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
<<<<<<< Updated upstream
=======

    m_isInhibitedFromApplet = NightLightInhibitor::instance().isInhibited();
    connect(&NightLightInhibitor::instance(), &NightLightInhibitor::inhibitedChanged, this, [this]() {
        setInhibitedFromApplet(NightLightInhibitor::instance().isInhibited());
    });
>>>>>>> Stashed changes
}

NightLightControlPrivate::~NightLightControlPrivate()
{
}

void NightLightControlPrivate::handlePropertiesChanged(const QString &interfaceName,
                                                       const QVariantMap &changedProperties,
                                                       const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    updateProperties(changedProperties);
}

int NightLightControlPrivate::currentTemperature() const
{
    return m_currentTemperature;
}

int NightLightControlPrivate::targetTemperature() const
{
    return m_targetTemperature;
}

void NightLightControlPrivate::updateProperties(const QVariantMap &properties)
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

    const QVariant inhibited = properties.value(QStringLiteral("inhibited"));
    if (inhibited.isValid()) {
        setInhibited(inhibited.toBool());
    }
}

void NightLightControlPrivate::setCurrentTemperature(int temperature)
{
    if (m_currentTemperature == temperature) {
        return;
    }
    m_currentTemperature = temperature;
    Q_EMIT currentTemperatureChanged();
}

void NightLightControlPrivate::setTargetTemperature(int temperature)
{
    if (m_targetTemperature == temperature) {
        return;
    }
    m_targetTemperature = temperature;
    Q_EMIT targetTemperatureChanged();
}

bool NightLightControlPrivate::isAvailable() const
{
    return m_isAvailable;
}

void NightLightControlPrivate::setAvailable(bool available)
{
    if (m_isAvailable == available) {
        return;
    }
    m_isAvailable = available;
    Q_EMIT availableChanged();
}

bool NightLightControlPrivate::isEnabled() const
{
    return m_isEnabled;
}

void NightLightControlPrivate::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled) {
        return;
    }
    m_isEnabled = enabled;
    Q_EMIT enabledChanged();
}

bool NightLightControlPrivate::isRunning() const
{
    return m_isRunning;
}

void NightLightControlPrivate::setRunning(bool running)
{
    if (m_isRunning == running) {
        return;
    }
    m_isRunning = running;
    Q_EMIT runningChanged();
}

bool NightLightControlPrivate::isInhibited() const
{
    return m_isInhibited;
}

void NightLightControlPrivate::setInhibited(bool inhibited)
{
    m_isInhibited = inhibited;
    Q_EMIT inhibitedChanged();
}

void NightLightControlPrivate::toggleInhibition()
{
    NightLightInhibitor::instance().toggleInhibition();
}

int NightLightControlPrivate::mode() const
{
    return m_mode;
}

void NightLightControlPrivate::setMode(int mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    Q_EMIT modeChanged();
}

bool NightLightControlPrivate::isDaylight() const
{
    return m_isDaylight;
}

void NightLightControlPrivate::setDaylight(bool daylight)
{
    if (m_isDaylight == daylight) {
        return;
    }
    m_isDaylight = daylight;
    Q_EMIT daylightChanged();
}

quint64 NightLightControlPrivate::currentTransitionEndTime() const
{
    return m_currentTransitionEndTime;
}

void NightLightControlPrivate::setCurrentTransitionEndTime(quint64 currentTransitionEndTime)
{
    if (m_currentTransitionEndTime == currentTransitionEndTime) {
        return;
    }
    m_currentTransitionEndTime = currentTransitionEndTime;
    Q_EMIT currentTransitionEndTimeChanged();
}

quint64 NightLightControlPrivate::scheduledTransitionStartTime() const
{
    return m_scheduledTransitionStartTime;
}

void NightLightControlPrivate::setScheduledTransitionStartTime(quint64 scheduledTransitionStartTime)
{
    if (m_scheduledTransitionStartTime == scheduledTransitionStartTime) {
        return;
    }
    m_scheduledTransitionStartTime = scheduledTransitionStartTime;
    Q_EMIT scheduledTransitionStartTimeChanged();
}

NightLightControl::NightLightControl(QObject *parent)
    : QObject(parent)
    , d(new NightLightControlPrivate(this))
{
    connect(d, &NightLightControlPrivate::availableChanged, this, &NightLightControl::availableChanged);
    connect(d, &NightLightControlPrivate::enabledChanged, this, &NightLightControl::enabledChanged);
    connect(d, &NightLightControlPrivate::runningChanged, this, &NightLightControl::runningChanged);
    connect(d, &NightLightControlPrivate::inhibitedChanged, this, &NightLightControl::inhibitedChanged);
    connect(d, &NightLightControlPrivate::modeChanged, this, &NightLightControl::modeChanged);
    connect(d, &NightLightControlPrivate::daylightChanged, this, &NightLightControl::daylightChanged);
    connect(d, &NightLightControlPrivate::currentTransitionEndTimeChanged, this, &NightLightControl::currentTransitionEndTimeChanged);
    connect(d, &NightLightControlPrivate::scheduledTransitionStartTimeChanged, this, &NightLightControl::scheduledTransitionStartTimeChanged);
    connect(d, &NightLightControlPrivate::currentTemperatureChanged, this, &NightLightControl::currentTemperatureChanged);
    connect(d, &NightLightControlPrivate::targetTemperatureChanged, this, &NightLightControl::targetTemperatureChanged);
}

NightLightControl::~NightLightControl()
{
}

bool NightLightControl::isAvailable() const
{
    return d->isAvailable();
}

bool NightLightControl::isEnabled() const
{
    return d->isEnabled();
}

bool NightLightControl::isRunning() const
{
    return d->isRunning();
}

bool NightLightControl::isInhibited() const
{
    return d->isInhibited();
}

void NightLightControl::toggleInhibition()
{
    d->toggleInhibition();
}

int NightLightControl::mode() const
{
    return d->mode();
}

bool NightLightControl::isDaylight() const
{
    return d->isDaylight();
}

int NightLightControl::currentTemperature() const
{
    return d->currentTemperature();
}

int NightLightControl::targetTemperature() const
{
    return d->targetTemperature();
}

quint64 NightLightControl::currentTransitionEndTime() const
{
    return d->currentTransitionEndTime();
}

quint64 NightLightControl::scheduledTransitionStartTime() const
{
    return d->scheduledTransitionStartTime();
}

#include "moc_nightlightcontrol.cpp"
