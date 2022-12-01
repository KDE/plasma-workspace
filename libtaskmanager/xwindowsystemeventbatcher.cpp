/*
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "xwindowsystemeventbatcher.h"

#include <QDebug>
#include <QTimerEvent>

#include <KX11Extras>

#define BATCH_TIME 10

static const NET::Properties s_cachableProperties = NET::WMName | NET::WMVisibleName;
static const NET::Properties2 s_cachableProperties2 = NET::WM2UserTime;

XWindowSystemEventBatcher::XWindowSystemEventBatcher(QObject *parent)
    : QObject(parent)
{
    connect(KX11Extras::self(), &KX11Extras::windowAdded, this, &XWindowSystemEventBatcher::windowAdded);

    // remove our cache entries when we lose a window, otherwise we might fire change signals after a window is destroyed which wouldn't make sense
    connect(KX11Extras::self(), &KX11Extras::windowRemoved, this, [this](WId wid) {
        m_cache.remove(wid);
        Q_EMIT windowRemoved(wid);
    });

    QObject::connect(KX11Extras::self(), &KX11Extras::windowChanged, this, [this](WId window, NET::Properties properties, NET::Properties2 properties2) {
        // if properties contained only cachable flags
        if ((properties | s_cachableProperties) == s_cachableProperties && (properties2 | s_cachableProperties2) == s_cachableProperties2) {
            m_cache[window].properties |= properties;
            m_cache[window].properties2 |= properties2;
            if (!m_timerId) {
                m_timerId = startTimer(BATCH_TIME);
            }
        } else {
            // submit all caches along with any real updates
            auto it = m_cache.constFind(window);
            if (it != m_cache.constEnd()) {
                properties |= it->properties;
                properties2 |= it->properties2;
                m_cache.erase(it);
            }
            Q_EMIT windowChanged(window, properties, properties2);
        }
    });
}

void XWindowSystemEventBatcher::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != m_timerId) {
        return;
    }
    for (auto it = m_cache.constBegin(); it != m_cache.constEnd(); it++) {
        Q_EMIT windowChanged(it.key(), it.value().properties, it.value().properties2);
    };
    m_cache.clear();
    killTimer(m_timerId);
    m_timerId = 0;
}
