/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <qqmlregistration.h>

class NightLightMonitorPrivate;

/**
 * The Monitor provides a way for monitoring the state of Night Light.
 */
class NightLightMonitor : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /**
     * This property holds a value to indicate if Night Light is available.
     */
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

    /**
     * This property holds a value to indicate if Night Light is enabled.
     */
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)

    /**
     * This property holds a value to indicate if Night Light is running.
     */
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

    /**
     * This property holds a value to indicate if Night Light is on day mode.
     */
    Q_PROPERTY(bool daylight READ isDaylight NOTIFY daylightChanged)

    /**
     * This property holds a value to indicate which mode is set for transitions (0 - automatic location, 1 - manual location, 2 - manual timings, 3 - constant)
     */
    Q_PROPERTY(int mode READ mode NOTIFY modeChanged)

    /**
     * This property holds a value to indicate currently applied color temperature.
     */
    Q_PROPERTY(int currentTemperature READ currentTemperature NOTIFY currentTemperatureChanged)

    /**
     * This property holds a value to indicate currently applied color temperature.
     */
    Q_PROPERTY(int targetTemperature READ targetTemperature NOTIFY targetTemperatureChanged)

    /**
     * This property holds a value to indicate the end time of the previous color transition in msec since epoch.
     */
    Q_PROPERTY(quint64 currentTransitionEndTime READ currentTransitionEndTime NOTIFY currentTransitionEndTimeChanged)

    /**
     * This property holds a value to indicate the start time of the next color transition in msec since epoch.
     */
    Q_PROPERTY(quint64 scheduledTransitionStartTime READ scheduledTransitionStartTime NOTIFY scheduledTransitionStartTimeChanged)

public:
    explicit NightLightMonitor(QObject *parent = nullptr);
    ~NightLightMonitor() override;

    /**
     * Returns @c true if Night Light is available; otherwise @c false.
     */
    bool isAvailable() const;

    /**
     * Returns @c true if Night Light is enabled; otherwise @c false.
     */
    bool isEnabled() const;

    /**
     * Returns @c true if Night Light is running; otherwise @c false.
     */
    bool isRunning() const;

    /**
     * Returns @c true if Night Light is on day mode; otherwise @c false.
     */
    bool isDaylight() const;

    /**
     * Returns 0 if automatic location, 1 if manual location, 2 if manual timings, 3 if constant
     */
    int mode() const;

    /**
     * Returns currently applied screen color temperature.
     */
    int currentTemperature() const;

    /**
     * Returns currently applied screen color temperature.
     */
    int targetTemperature() const;

    /**
     * Returns the time of the end of the previous color transition in msec since epoch.
     */
    quint64 currentTransitionEndTime() const;

    /**
     * Returns the time of the start of the next color transition in msec since epoch.
     */
    quint64 scheduledTransitionStartTime() const;

Q_SIGNALS:
    /**
     * This signal is emitted when Night Light becomes (un)available.
     */
    void availableChanged();

    /**
     * Emitted whenever Night Light is enabled or disabled.
     */
    void enabledChanged();

    /**
     * Emitted whenever Night Light starts or stops running.
     */
    void runningChanged();

    /**
     * Emitted whenever Night Light changes between day and night time.
     */
    void daylightChanged();

    /**
     * Emitted whenever Night Light timings mode changes.
     */
    void modeChanged();

    /**
     * Emitted whenever the current screen color temperature has changed.
     */
    void currentTemperatureChanged();

    /**
     * Emitted whenever the current screen color temperature has changed.
     */
    void targetTemperatureChanged();

    /**
     * Emitted when the end time of the previous color transition has changed.
     */
    void currentTransitionEndTimeChanged();

    /**
     * Emitted when the end time of the next color transition has changed.
     */
    void scheduledTransitionStartTimeChanged();

private:
    NightLightMonitorPrivate *d;
};
