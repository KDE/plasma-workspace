/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowvisibilitymonitor.h"

WindowVisibilityMonitor::WindowVisibilityMonitor(QObject *parent)
    : QObject(parent)
{
}

WindowVisibilityMonitor::~WindowVisibilityMonitor() = default;

bool WindowVisibilityMonitor::exposed() const
{
    return m_exposed;
}

QQuickWindow *WindowVisibilityMonitor::window() const
{
    return m_window;
}

void WindowVisibilityMonitor::setWindow(QQuickWindow *window)
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
    updateExposed();
}

bool WindowVisibilityMonitor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window && event->type() == QEvent::Expose) {
        updateExposed();
    }

    return QObject::eventFilter(watched, event);
}

void WindowVisibilityMonitor::updateExposed()
{
    const bool exposed = m_window && m_window->isExposed();
    if (m_exposed == exposed) {
        return;
    }

    m_exposed = exposed;
    Q_EMIT exposedChanged();
}
