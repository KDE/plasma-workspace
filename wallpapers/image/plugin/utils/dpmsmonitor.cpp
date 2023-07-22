/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dpmsmonitor.h"

DPMSMonitor::DPMSMonitor(QObject *parent)
    : KScreen::Dpms(parent)
{
    connect(this, &KScreen::Dpms::modeChanged, this, &DPMSMonitor::onModeChanged);
}

DPMSMonitor::~DPMSMonitor()
{
}

bool DPMSMonitor::monitorOn() const
{
    return m_monitorOn;
}

QQuickWindow *DPMSMonitor::window() const
{
    return m_window;
}

void DPMSMonitor::setWindow(QQuickWindow *window)
{
    if (m_window == window) {
        return;
    }

    if (m_window) {
        disconnect(m_window, &QQuickWindow::screenChanged, this, &DPMSMonitor::onScreenChanged);
        m_screen = nullptr;
    }
    if (window) {
        connect(window, &QQuickWindow::screenChanged, this, &DPMSMonitor::onScreenChanged);
        m_screen = window->screen();
    }

    m_window = window;
    Q_EMIT windowChanged();
}

void DPMSMonitor::onScreenChanged(QScreen *screen)
{
    m_screen = screen;
}

void DPMSMonitor::onModeChanged(KScreen::Dpms::Mode mode, QScreen *screen)
{
    if (screen != m_screen) {
        return;
    }

    const bool monitorOn = (mode == KScreen::Dpms::On);
    if (m_monitorOn == monitorOn) {
        return;
    }

    m_monitorOn = monitorOn;
    Q_EMIT monitorOnChanged();
}
