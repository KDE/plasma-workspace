/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "idletimeout.h"

#include <KIdleTime>

IdleTimeout::IdleTimeout(std::chrono::milliseconds interval, QObject *parent)
    : QObject(parent)
{
    KIdleTime *idleTime = KIdleTime::instance();
    connect(idleTime, &KIdleTime::timeoutReached, this, [this](int identifier) {
        if (m_notifierId == identifier) {
            Q_EMIT timeout();
        }
    });

    m_notifierId = idleTime->addIdleTimeout(interval);
}

IdleTimeout::~IdleTimeout()
{
    KIdleTime::instance()->removeIdleTimeout(m_notifierId);
}

#include "moc_idletimeout.cpp"
