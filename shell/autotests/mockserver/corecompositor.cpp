/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "corecompositor.h"
#include <thread>

namespace MockCompositor
{
CoreCompositor::CoreCompositor()
    : m_display(wl_display_create())
    , m_socketName(wl_display_add_socket_auto(m_display))
    , m_eventLoop(wl_display_get_event_loop(m_display))

    // Start dispatching
    , m_dispatchThread([this]() {
        while (m_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            dispatch();
        }
    })
{
    qputenv("WAYLAND_DISPLAY", m_socketName);
    m_timer.start();
    Q_ASSERT(isClean());
}

CoreCompositor::~CoreCompositor()
{
    m_running = false;
    m_dispatchThread.join();
    wl_display_destroy(m_display);
}

bool CoreCompositor::isClean()
{
    Lock lock(this);
    for (auto *global : qAsConst(m_globals)) {
        if (!global->isClean())
            return false;
    }
    return true;
}

QString CoreCompositor::dirtyMessage()
{
    Lock lock(this);
    QStringList messages;
    for (auto *global : qAsConst(m_globals)) {
        if (!global->isClean())
            messages << (global->metaObject()->className() % QLatin1String(": ") % global->dirtyMessage());
    }
    return messages.join(", ");
}

void CoreCompositor::dispatch()
{
    Lock lock(this);
    wl_display_flush_clients(m_display);
    constexpr int timeout = 0; // immediate return
    wl_event_loop_dispatch(m_eventLoop, timeout);
}

/*!
 * \brief Adds a new global interface for the compositor
 *
 * Takes ownership of \a global
 */
void CoreCompositor::add(Global *global)
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    m_globals.append(global);
}

void CoreCompositor::remove(Global *global)
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    m_globals.removeAll(global);
    delete global;
}

uint CoreCompositor::nextSerial()
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    return wl_display_next_serial(m_display);
}

uint CoreCompositor::currentTimeMilliseconds()
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    return uint(m_timer.elapsed());
}

wl_client *CoreCompositor::client(int index)
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    wl_list *clients = wl_display_get_client_list(m_display);
    wl_client *client = nullptr;
    int i = 0;
    wl_client_for_each(client, clients)
    {
        if (i++ == index)
            return client;
    }
    return nullptr;
}

void CoreCompositor::warnIfNotLockedByThread(const char *caller)
{
    if (!m_lock || !m_lock->isOwnedByCurrentThread()) {
        qWarning() << caller << "called without locking the compositor to the current thread."
                   << "This means the compositor can start dispatching at any moment,"
                   << "potentially leading to threading issues."
                   << "Unless you know what you are doing you should probably fix the test"
                   << "by locking the compositor before accessing it (see mutex()).";
    }
}

} // namespace MockCompositor
