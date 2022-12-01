/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowsystem.h"

#include <QQuickItem>

#include <KWindowSystem>
#include <KX11Extras>

WindowSystem::WindowSystem(QObject *parent)
    : QObject(parent)
{
}

WindowSystem::~WindowSystem()
{
}

bool WindowSystem::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        removeEventFilter(watched);
        Q_EMIT focusIn(qobject_cast<QQuickWindow *>(watched));
    }

    return false;
}

void WindowSystem::forceActive(QQuickItem *item)
{
    if (!item || !item->window()) {
        return;
    }

    KX11Extras::forceActiveWindow(item->window()->winId());
    KWindowSystem::raiseWindow(item->window()->winId());
}

bool WindowSystem::isActive(QQuickItem *item)
{
    if (!item || !item->window()) {
        return false;
    }

    return item->window()->isActive();
}

void WindowSystem::monitorWindowFocus(QQuickItem *item)
{
    if (!item || !item->window()) {
        return;
    }

    item->window()->installEventFilter(this);
}

void WindowSystem::monitorWindowVisibility(QQuickItem *item)
{
    if (!item || !item->window()) {
        return;
    }

    connect(item->window(), &QQuickWindow::visibilityChanged, this, &WindowSystem::monitoredWindowVisibilityChanged, Qt::UniqueConnection);
}

void WindowSystem::monitoredWindowVisibilityChanged(QWindow::Visibility visibility) const
{
    bool visible = (visibility != QWindow::Hidden);
    QQuickWindow *w = static_cast<QQuickWindow *>(QObject::sender());

    if (!visible) {
        Q_EMIT hidden(w);
    }
}
