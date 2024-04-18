/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

class NightLightControlPrivate : public QObject
{
    Q_OBJECT

public:
    explicit NightLightControlPrivate(QObject *parent = nullptr);
    ~NightLightControlPrivate() override;

    int currentTemperature() const;
    int targetTemperature() const;
    quint64 currentTransitionEndTime() const;
    quint64 scheduledTransitionStartTime() const;
    bool isDaylight() const;
    int mode() const;
    bool isAvailable() const;
    bool isEnabled() const;
    bool isRunning() const;
    bool isInhibited() const;
    bool isInhibitedFromApplet() const;
public Q_SLOTS:
    void toggleInhibition();
Q_SIGNALS:
    void currentTemperatureChanged();
    void targetTemperatureChanged();
    void currentTransitionEndTimeChanged();
    void scheduledTransitionStartTimeChanged();
    void daylightChanged();
    void modeChanged();
    void availableChanged();
    void enabledChanged();
    void runningChanged();
    void inhibitedChanged();
    void inhibitedFromAppletChanged();

private Q_SLOTS:
    void handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    void updateProperties(const QVariantMap &properties);
    void setCurrentTemperature(int temperature);
    void setTargetTemperature(int temperature);
    void setCurrentTransitionEndTime(quint64 currentTransitionEndTime);
    void setScheduledTransitionStartTime(quint64 scheduledTransitionStartTime);
    void setDaylight(bool daylight);
    void setMode(int mode);
    void setAvailable(bool available);
    void setEnabled(bool enabled);
    void setRunning(bool running);
    void setInhibited(bool inhibited);
    void setInhibitedFromApplet(bool inhibitedFromApplet);

    int m_currentTemperature = 0;
    int m_targetTemperature = 0;
    quint64 m_currentTransitionEndTime = 0;
    quint64 m_scheduledTransitionStartTime = 0;
    bool m_isDaylight = false;
    int m_mode = 0;
    bool m_isAvailable = false;
    bool m_isEnabled = false;
    bool m_isRunning = false;
    bool m_isInhibited = false;
    bool m_isInhibitedFromApplet = false;
};
