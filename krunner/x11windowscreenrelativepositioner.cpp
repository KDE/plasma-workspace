/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "x11windowscreenrelativepositioner.h"

#include <QScreen>

#include <KWindowSystem>

X11WindowScreenRelativePositioner::X11WindowScreenRelativePositioner(QWindow *window)
    : QObject(window)
    , m_window(window)
{
    if (!KWindowSystem::isPlatformX11()) {
        qCritical("X11WindowScreenRelativePositioner should only be used on X11 windows");
    }

    m_window->installEventFilter(this);
    connect(m_window, &QWindow::screenChanged, this, &X11WindowScreenRelativePositioner::handleScreenChanged);

    handleScreenChanged();
}

void X11WindowScreenRelativePositioner::setAnchors(Qt::Edges anchors)
{
    if (m_anchors == anchors) {
        return;
    }

    m_anchors = anchors;
    Q_EMIT anchorsChanged();
    polish();
}

void X11WindowScreenRelativePositioner::setMargins(const QMargins &margins)
{
    if (m_margins == margins) {
        return;
    }
    m_margins = margins;
    Q_EMIT marginsChanged();
    polish();
}

bool X11WindowScreenRelativePositioner::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_window) {
        return false;
    }
    switch (event->type()) {
    case QEvent::UpdateRequest:
        updatePolish();
        break;
    case QEvent::Resize:
    case QEvent::Expose:
        polish();
        break;
    default:
        break;
    }
    return false;
}

void X11WindowScreenRelativePositioner::polish()
{
    m_needsRepositioning = true;
    m_window->requestUpdate();
}

void X11WindowScreenRelativePositioner::updatePolish()
{
    if (m_needsRepositioning) {
        reposition();
        m_needsRepositioning = false;
    }
}

void X11WindowScreenRelativePositioner::reposition()
{
    QScreen *screen = m_window->screen();

    QRect screenRect = screen->availableGeometry();
    screenRect = screenRect.marginsRemoved(m_margins);

    QRect targetRect;
    // Y
    if (m_anchors.testFlags({Qt::TopEdge, Qt::BottomEdge})) {
        targetRect.setTop(screenRect.top());
        targetRect.setBottom(screenRect.bottom());
    } else {
        targetRect.setHeight(m_window->height());
        if (m_anchors.testFlag(Qt::TopEdge)) {
            targetRect.moveTop(screenRect.top());
        } else if (m_anchors.testFlag(Qt::BottomEdge)) {
            targetRect.moveBottom(screenRect.bottom());
        } else {
            targetRect.moveCenter(QPoint(0, screenRect.center().y()));
        }
    }

    // X
    if (m_anchors.testFlags({Qt::LeftEdge, Qt::RightEdge})) {
        targetRect.setLeft(screenRect.left());
        targetRect.setRight(screenRect.right());
    } else {
        targetRect.setWidth(m_window->width());
        if (m_anchors.testFlag(Qt::LeftEdge)) {
            targetRect.moveLeft(screenRect.left());
        } else if (m_anchors.testFlag(Qt::RightEdge)) {
            targetRect.moveRight(screenRect.right());
        } else {
            targetRect.moveCenter(QPoint(screenRect.center().x(), targetRect.center().y()));
        }
    }

    m_window->setGeometry(targetRect);
}

void X11WindowScreenRelativePositioner::handleScreenChanged()
{
    QScreen *newScreen = m_window->screen();
    if (newScreen == m_screen) {
        return;
    }
    if (m_screen) {
        disconnect(m_screen, nullptr, this, nullptr);
    }
    m_screen = newScreen;
    connect(newScreen, &QScreen::availableGeometryChanged, this, &X11WindowScreenRelativePositioner::polish);
    polish();
}
