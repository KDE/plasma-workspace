/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "alignedtimer.h"

#include <fcntl.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <QDebug>
#include <QSocketNotifier>
#include <qsystemdetection.h>

#ifndef Q_OS_LINUX
#include <QDBusConnection>
#endif

// these are not exposed in glibc universally hence redeclared

#ifndef TFD_TIMER_ABSTIME
#define TFD_TIMER_ABSTIME 0x1
#endif

#ifndef TFD_TIMER_CANCEL_ON_SET
#define TFD_TIMER_CANCEL_ON_SET 0x2
#endif

std::shared_ptr<AlignedTimer> AlignedTimer::getMinuteTimer()
{
    static std::weak_ptr<AlignedTimer> s_minuteTimer;

    std::shared_ptr<AlignedTimer> timer = s_minuteTimer.lock();
    if (!timer) {
        timer = std::make_shared<AlignedTimer>(60);
        s_minuteTimer = timer;
    }
    return timer;
}

std::shared_ptr<AlignedTimer> AlignedTimer::getSecondTimer()
{
    static std::weak_ptr<AlignedTimer> s_secondTimer;

    std::shared_ptr<AlignedTimer> timer = s_secondTimer.lock();
    if (!timer) {
        timer = std::make_shared<AlignedTimer>(1);
        s_secondTimer = timer;
    }
    return timer;
}

AlignedTimer::AlignedTimer(int interval)
    : m_interval(interval)
{
    m_timerFd = timerfd_create(CLOCK_REALTIME, O_CLOEXEC | O_NONBLOCK);

    auto notifier = new QSocketNotifier(m_timerFd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this](int fd) {
        uint64_t c = 0;
        read(fd, &c, sizeof(c));
        if (c == 0) { // clock skew detected
            initTimer();
        }
        Q_EMIT timeout();
    });
    initTimer();
}

AlignedTimer::~AlignedTimer()
{
    if (m_timerFd > 0) {
        close(m_timerFd);
    }
}

void AlignedTimer::initTimer()
{
    itimerspec timespec = {{0, 0}};
    timespec.it_value.tv_sec = m_interval;
    timespec.it_interval.tv_sec = m_interval;

    const int flags = TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET;

    const int err = timerfd_settime(m_timerFd, flags, &timespec, nullptr);
    if (err) {
        qWarning() << "Could not create timer with TFD_TIMER_CANCEL_ON_SET. Clock skews will not be detected. Error:" << qPrintable(strerror(err));
        return;
    }
}
