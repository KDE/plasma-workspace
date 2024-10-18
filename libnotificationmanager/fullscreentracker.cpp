/*
    SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "fullscreentracker_p.h"

#include <QGuiApplication>
#include <QWindow>

#include <KWindowInfo>
#include <KWindowSystem>
#include <qguiapplication.h>

#include "debug.h"

using namespace NotificationManager;

FullscreenTracker::FullscreenTracker()
    : QObject(nullptr)
{
    checkFullscreenActive();

    connect(qApp, &QGuiApplication::focusWindowChanged, this, [this](QWindow *window) {
        Q_UNUSED(window);
        checkFullscreenActive();
    });
}

FullscreenTracker::~FullscreenTracker() = default;

FullscreenTracker::Ptr FullscreenTracker::createTracker()
{
    static std::weak_ptr<FullscreenTracker> s_instance;
    if (s_instance.expired()) {
        std::shared_ptr<FullscreenTracker> ptr(new FullscreenTracker());
        s_instance = ptr;
        return ptr;
    }
    return s_instance.lock();
}

bool FullscreenTracker::fullscreenActive() const
{
    return m_fullscreenActive;
}

void FullscreenTracker::setFullscreenActive(bool active)
{
    if (m_fullscreenActive != active) {
        m_fullscreenActive = active;
        Q_EMIT fullscreenActiveChanged(active);
    }
}

void FullscreenTracker::checkFullscreenActive()
{
    QWindow *window = QGuiApplication::focusWindow();
    if (!window) {
        setFullscreenActive(false);
        return;
    }

    KWindowInfo info(window->winId(), NET::WMState | NET::WMWindowType);
    if (info.windowType(NET::AllTypesMask) && info.state() & NET::FullScreen) {
        setFullscreenActive(true);
    } else {
        setFullscreenActive(false);
    }
}
