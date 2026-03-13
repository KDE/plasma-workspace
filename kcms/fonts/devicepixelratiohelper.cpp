/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "devicepixelratiohelper.h"

DevicePixelRatioHelper::DevicePixelRatioHelper(QObject *parent)
    : QObject(parent)
{
}

void DevicePixelRatioHelper::setWindow(QQuickWindow *window)
{
    if (m_window == window) {
        return;
    }

    if (m_window) {
        m_window->removeEventFilter(this);
    }

    m_window = window;

    if (m_window) {
        m_window->installEventFilter(this);
    }

    Q_EMIT windowChanged();
    updateDevicePixelRatio();
}

QQuickWindow *DevicePixelRatioHelper::window() const
{
    return m_window;
}

qreal DevicePixelRatioHelper::devicePixelRatio() const
{
    return m_devicePixelRatio;
}

bool DevicePixelRatioHelper::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::DevicePixelRatioChange) {
        updateDevicePixelRatio();
    }

    return QObject::eventFilter(obj, event);
}

void DevicePixelRatioHelper::updateDevicePixelRatio()
{
    if (!m_window) {
        return;
    }

    const qreal devicePixelRatio = m_window->devicePixelRatio();
    if (m_devicePixelRatio != devicePixelRatio) {
        m_devicePixelRatio = devicePixelRatio;
        Q_EMIT devicePixelRatioChanged();
    }
}
