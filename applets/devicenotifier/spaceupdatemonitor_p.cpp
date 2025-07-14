/*
 * SPDX-FileCopyrightText: 2025 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "spaceupdatemonitor_p.h"

#include "devicenotifier_debug.h"

#include <QTimer>

using namespace std::chrono_literals;

inline constexpr auto UPDATE_INTERVAL = 1min;

std::shared_ptr<SpaceUpdateMonitor> SpaceUpdateMonitor::instance()
{
    static std::weak_ptr<SpaceUpdateMonitor> s_clip;
    if (s_clip.expired()) {
        std::shared_ptr<SpaceUpdateMonitor> ptr{new SpaceUpdateMonitor};
        s_clip = ptr;
        return ptr;
    }
    return s_clip.lock();
}

SpaceUpdateMonitor::SpaceUpdateMonitor(QObject *parent)
    : QObject(parent)
    , m_usageCount(0)
    , m_spaceWatcher(new QTimer(this))
{
    m_spaceWatcher->setSingleShot(true);
    m_spaceWatcher->setInterval(UPDATE_INTERVAL);
    connect(m_spaceWatcher, &QTimer::timeout, this, &SpaceUpdateMonitor::updateSpace);

    qCDebug(APPLETS::DEVICENOTIFIER) << "SpaceUpdateMonitor: created";
}

SpaceUpdateMonitor::~SpaceUpdateMonitor()
{
    m_spaceWatcher->stop();

    qCDebug(APPLETS::DEVICENOTIFIER) << "SpaceUpdateMonitor: removed";
}

void SpaceUpdateMonitor::setIsVisible(bool visible)
{
    if (visible) {
        ++m_usageCount;
    } else {
        --m_usageCount;
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "SpaceUpdateMonitor: usage counter updated. Usage count: " << m_usageCount;

    if (m_usageCount == 0) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "SpaceUpdateMonitor: Stop timer";
        m_spaceWatcher->setSingleShot(true);
        return;
    }

    m_spaceWatcher->setSingleShot(false);
    if (!m_spaceWatcher->isActive()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "SpaceUpdateMonitor: Start timer";
        m_spaceWatcher->start();
    }
}

#include "moc_spaceupdatemonitor_p.cpp"
