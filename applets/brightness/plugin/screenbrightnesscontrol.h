/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QBindable>
#include <QCoroTask>
#include <QObject>
#include <qqmlregistration.h>

class QDBusPendingCallWatcher;

class ScreenBrightnessControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int brightness READ default WRITE setBrightness NOTIFY brightnessChanged BINDABLE bindableBrightness)
    Q_PROPERTY(int brightnessMax READ default NOTIFY brightnessMaxChanged BINDABLE bindableBrightnessMax)
    Q_PROPERTY(bool isBrightnessAvailable READ default NOTIFY isBrightnessAvailableChanged BINDABLE bindableIsBrightnessAvailable)
    Q_PROPERTY(bool isSilent WRITE setIsSilent)

public:
    explicit ScreenBrightnessControl(QObject *parent = nullptr);
    ~ScreenBrightnessControl() override;

    QBindable<bool> bindableIsBrightnessAvailable();
    QBindable<int> bindableBrightness();
    QBindable<int> bindableBrightnessMax();

    void setBrightness(int value);
    void setIsSilent(bool status);

Q_SIGNALS:
    void brightnessChanged(int value);
    void brightnessMaxChanged(int value);
    void isBrightnessAvailableChanged(bool status);

private Q_SLOTS:
    void onBrightnessChanged(int value);
    void onBrightnessMaxChanged(int value);

private:
    QCoro::Task<void> init();

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ScreenBrightnessControl, bool, m_isBrightnessAvailable, false, &ScreenBrightnessControl::isBrightnessAvailableChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ScreenBrightnessControl, int, m_brightness, 0, &ScreenBrightnessControl::brightnessChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ScreenBrightnessControl, int, m_maxBrightness, 0, &ScreenBrightnessControl::brightnessMaxChanged);

    std::unique_ptr<QDBusPendingCallWatcher> m_brightnessChangeWatcher;
    bool m_isSilent = false;
};
