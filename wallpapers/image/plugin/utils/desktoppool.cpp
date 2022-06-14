/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "desktoppool.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>

#include <KWindowSystem>

#include "debug.h"

DesktopPool::DesktopPool(QObject *parent)
    : QObject(parent)
{
    m_geometryChangedTimer.setSingleShot(true);
    m_geometryChangedTimer.setInterval(0);
    connect(&m_geometryChangedTimer, &QTimer::timeout, this, &DesktopPool::geometryChanged);
}

QUrl DesktopPool::globalImage(QQuickWindow *window) const
{
    std::shared_lock lock(m_lock);

    auto it = m_windowMap.find(window);

    if (it == m_windowMap.end()) {
        return {};
    }

    return it->second;
}

void DesktopPool::setGlobalImage(QQuickWindow *window, const QUrl &url)
{
    m_lock.lock();

    auto it = m_windowMap.find(window);

    if (it == m_windowMap.end()) {
        m_lock.unlock();
        setDesktop(window, url);
        return;
    }

    if (it->second == url) {
        m_lock.unlock();
        return;
    }

    it->second = url;
    qCDebug(IMAGEWALLPAPER) << "New global image:" << url;

    m_lock.unlock();

    // Update other windows
    Q_EMIT desktopWindowChanged(window);
}

QRect DesktopPool::boundingRect(const QUrl &url) const
{
    std::shared_lock lock(m_lock);

    const bool isX11 = KWindowSystem::isPlatformX11();

    return std::accumulate(m_windowMap.cbegin(), m_windowMap.cend(), QRect(), [&url, isX11](const QRect &rect, const std::pair<QQuickWindow *, QUrl> &pr) {
        if (pr.second == url) {
            if (isX11) {
                // On X11, x and y do not consider the scaling factor unlike on Wayland.
                const QRect r = pr.first->geometry();
                const double scaleFactor = pr.first->screen()->devicePixelRatio();
                return rect.united(QRect(r.topLeft(), r.size() * scaleFactor));
            }

            return rect.united(pr.first->geometry());
        }

        return rect;
    });
}

void DesktopPool::setDesktop(QQuickWindow *window, const QUrl &url)
{
    m_lock.lock();

    auto it = m_windowMap.find(window);

    if (it == m_windowMap.end()) {
        m_windowMap.emplace(window, url);
        void (QTimer::*startTimer)() = &QTimer::start;
        connect(window, &QWindow::xChanged, &m_geometryChangedTimer, startTimer);
        connect(window, &QWindow::yChanged, &m_geometryChangedTimer, startTimer);
        connect(window, &QWindow::widthChanged, &m_geometryChangedTimer, startTimer);
        connect(window, &QWindow::heightChanged, &m_geometryChangedTimer, startTimer);
    } else if (it->second != url) {
        it->second = url;
    } else {
        m_lock.unlock();
        return;
    }

    m_lock.unlock();

    Q_EMIT desktopWindowChanged(window);
}

void DesktopPool::unsetDesktop(QQuickWindow *window)
{
    m_lock.lock();

    auto it = m_windowMap.find(window);

    if (it == m_windowMap.end()) {
        // Invalid window geometry
        m_lock.unlock();
        return;
    }

    disconnect(window, nullptr, this, nullptr);
    m_windowMap.erase(it);

    m_lock.unlock();

    Q_EMIT desktopWindowChanged(window);

    return;
}
