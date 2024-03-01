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

class KeyboardBrightnessControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int brightness READ default WRITE setBrightness NOTIFY brightnessChanged BINDABLE bindableBrightness)
    Q_PROPERTY(int brightnessMax READ default NOTIFY brightnessMaxChanged BINDABLE bindableBrightnessMax)
    Q_PROPERTY(bool isBrightnessAvailable READ default NOTIFY isBrightnessAvailableChanged BINDABLE bindableIsBrightnessAvailable)
    Q_PROPERTY(bool isSilent WRITE setIsSilent)

public:
    explicit KeyboardBrightnessControl(QObject *parent = nullptr);
    ~KeyboardBrightnessControl() override;

    void setBrightness(int value);
    void setIsSilent(bool silent);

    QBindable<bool> bindableIsBrightnessAvailable();
    QBindable<int> bindableBrightness();
    QBindable<int> bindableBrightnessMax();

Q_SIGNALS:
    void brightnessChanged(int value);
    void brightnessMaxChanged(int value);
    void isBrightnessAvailableChanged(bool status);

private Q_SLOTS:
    void onBrightnessChanged(int value);
    void onBrightnessMaxChanged(int value);

private:
    QCoro::Task<void> init();

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(KeyboardBrightnessControl,
                                         bool,
                                         m_isBrightnessAvailable,
                                         false,
                                         &KeyboardBrightnessControl::isBrightnessAvailableChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(KeyboardBrightnessControl, int, m_brightness, 0, &KeyboardBrightnessControl::brightnessChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(KeyboardBrightnessControl, int, m_maxBrightness, 0, &KeyboardBrightnessControl::brightnessMaxChanged);

    bool m_isSilent = false;
};
