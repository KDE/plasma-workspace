/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2026 Rafael Sadowski <rafael@sizeofvoid.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
    SPDX-License-Identifier: ISC
*/

#include "alignedtimer.h"

#include <QDebug>
#include <QTimer>

#include <algorithm>
#include <climits>
#include <time.h>

std::shared_ptr<AlignedTimer> AlignedTimer::getMinuteTimer()
{
    static std::weak_ptr<AlignedTimer> s_minuteTimer;

    std::shared_ptr<AlignedTimer> timer = s_minuteTimer.lock();
    if (!timer) {
        timer = std::make_shared<AlignedTimer>(std::chrono::minutes(1));
        s_minuteTimer = timer;
    }
    return timer;
}

std::shared_ptr<AlignedTimer> AlignedTimer::getSecondTimer()
{
    static std::weak_ptr<AlignedTimer> s_secondTimer;

    std::shared_ptr<AlignedTimer> timer = s_secondTimer.lock();
    if (!timer) {
        timer = std::make_shared<AlignedTimer>(std::chrono::seconds(1));
        s_secondTimer = timer;
    }
    return timer;
}

AlignedTimer::AlignedTimer(std::chrono::seconds interval)
    : m_interval(interval)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        Q_EMIT timeout();
        initTimer();
    });
    initTimer();
}

AlignedTimer::~AlignedTimer() = default;

/**
 * Starts the timer so it fires exactly at the next whole interval boundary.
 * For example, with a 1-second interval the timer always fires at :00, :01...
 * and with a 1-minute interval it fires at 12:01:00, 12:02:00 regardless of
 * when this function is called.
 */
void AlignedTimer::initTimer()
{
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
        qCritical() << "AlignedTimer: clock_gettime failed.";
        return;
    }

    // Current wall clock time in milliseconds since the Unix epoch.
    const long long intervalMs = m_interval.count() * 1000LL;
    const long long nowMs = (long long)now.tv_sec * 1000LL + now.tv_nsec / 1000000LL;

    // Round down to the last boundary, then step forward to the next one.
    const long long nextMs = ((nowMs / intervalMs) + 1) * intervalMs;

    // How long to wait from now until that boundary.
    const long long delayMs = nextMs - nowMs;

    m_timer->start(static_cast<int>(std::min<long long>(delayMs, INT_MAX)));
}

#include "moc_alignedtimer.cpp"
