/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <qqmlregistration.h>

/**
 * The Control provides a way for controling the state of Night Light.
 */
class NightLightControl : public QObject
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
     * This property holds a value to indicate whether night light is currently inhibited.
     */
    Q_PROPERTY(bool inhibited READ isInhibited NOTIFY inhibitedChanged)

    /**
     * This property holds a value to indicate whether night light is currently inhibited from the applet can be uninhibited through it.
     */
    Q_PROPERTY(bool inhibitedFromApplet READ isInhibitedFromApplet NOTIFY inhibitedFromAppletChanged)

    /**
     * This property holds a value to indicate which mode is set for transitions (0 - automatic location, 1 - manual location, 2 - manual timings, 3 - constant)
     */
    Q_PROPERTY(int mode READ mode NOTIFY modeChanged)

    /**
     * This property holds a value to indicate if Night Light is on day mode.
     */
    Q_PROPERTY(bool daylight READ isDaylight NOTIFY daylightChanged)

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
    explicit NightLightControl(QObject *parent = nullptr);
    ~NightLightControl() override;

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
     * Returns @c true if Night Light is currently inhibited or inhibiting; otherwise @c false.
     */
    bool isInhibited() const;

    /**
     * Returns @c true if Night Light is currently inhibited from the applet and can be uninhibited; otherwise @c false.
     */
    bool isInhibitedFromApplet() const;

    /**
     * Returns 0 if automatic location, 1 if manual location, 2 if manual timings, 3 if constant
     */
    int mode() const;

    /**
     * Returns @c true if Night Light is on day mode; otherwise @c false.
     */
    bool isDaylight() const;

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

public Q_SLOTS:
    /**
     * Inhibits Night Light if currently running, and un-inhibits it if currently inhibited.
     */
    void toggleInhibition();

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
     * Emitted when the inhibition state of Night Light has changed.
     */
    void inhibitedChanged();

    /**
     * Emitted wgeb Night Light has been inhibited or uninnhibited from the applet.
     */
    void inhibitedFromAppletChanged();

    /**
     * Emitted whenever Night Light timings mode changes.
     */
    void modeChanged();

    /**
     * Emitted whenever Night Light changes between day and night time.
     */
    void daylightChanged();

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

private Q_SLOTS:
    void handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    void updateProperties(const QVariantMap &properties);
    void setAvailable(bool available);
    void setEnabled(bool enabled);
    void setRunning(bool running);
    void setInhibited(bool inhibited);
    void setInhibitedFromApplet(bool inhibitedFromApplet);
    void setMode(int mode);
    void setDaylight(bool daylight);
    void setCurrentTemperature(int temperature);
    void setTargetTemperature(int temperature);
    void setCurrentTransitionEndTime(quint64 currentTransitionEndTime);
    void setScheduledTransitionStartTime(quint64 scheduledTransitionStartTime);

    bool m_isAvailable = false;
    bool m_isEnabled = false;
    bool m_isRunning = false;
    bool m_isInhibited = false;
    bool m_isInhibitedFromApplet = false;
    int m_mode = 0;
    bool m_isDaylight = false;
    int m_currentTemperature = 0;
    int m_targetTemperature = 0;
    quint64 m_currentTransitionEndTime = 0;
    quint64 m_scheduledTransitionStartTime = 0;
};
