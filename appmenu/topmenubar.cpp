/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include "topmenubar.h"
#include "glowbar.h"

//KDE
#include <Plasma/Svg>
#include <KWindowSystem>

// Qt
#include <QMenu>
#include <QTimer>
#include <QDebug>
#include <QApplication>
#include <QPropertyAnimation>
#include <QDesktopWidget>

TopMenuBar::TopMenuBar()
    : MenuBar(),
    m_prevCursorPos(-1, -1),
    m_mouseTracker(new QTimer(this)),
    m_hideGlowTimer(new QTimer(this)),
    m_glowBar(new GlowBar())
{
    connect(this, SIGNAL(aboutToHide()), this, SLOT(slotAboutToHide()));
    connect(m_mouseTracker, SIGNAL(timeout()), this, SLOT(slotMouseTracker()));
    connect(m_hideGlowTimer, SIGNAL(timeout()), this, SLOT(slotHideGlowBar()));
}

TopMenuBar::~TopMenuBar()
{
    delete m_mouseTracker;
    delete m_hideGlowTimer;
    hideGlowBar();
    delete m_glowBar;
}

void TopMenuBar::enableMouseTracking(bool enable)
{
    if (enable) {
        if (!cursorInMenuBar()) {
            showGlowBar();
        }
        m_mouseTracker->start(250);
    } else {
        hideGlowBar();
        m_mouseTracker->stop();
    }
}

void TopMenuBar::updateSize()
{
    // Enable mouse tracking on resize if needed
    if (!m_mouseTracker->isActive() && !cursorInMenuBar()) {
        enableMouseTracking();
    }
    resize(sizeHint());
}

void TopMenuBar::move(QPoint p)
{
    MenuBar::move(p);
    if (m_glowBar) {
        m_glowBar->move(p);
        m_glowBar->setPixmap(triggerRect().topLeft(), triggerRect().width());
    }
}

bool TopMenuBar::cursorInMenuBar()
{
    if (m_mouseTracker->isActive()) {
        return triggerRect().contains(QCursor::pos());
    } else {
        return MenuBar::cursorInMenuBar();
    }
}

void TopMenuBar::slotAboutToHide()
{
        enableMouseTracking();
}

void TopMenuBar::slotMouseTracker()
{
    QPoint cursorPos = QCursor::pos();

    // reset timer
    if (cursorPos != m_prevCursorPos && m_hideGlowTimer->isActive()) {
        m_hideGlowTimer->stop();
        m_hideGlowTimer->start(10000);
    }

    if (cursorInMenuBar()) { // show menubar
        m_mouseTracker->stop();
        hideGlowBar();
        show();
    } else if(cursorPos != m_prevCursorPos) { // change glowbar opacity
        qreal opacity = glowBarOpacity();
        QPropertyAnimation *anim = new QPropertyAnimation(m_glowBar, "windowOpacity");
        anim->setStartValue(m_glowBar->windowOpacity());
        anim->setEndValue(opacity);
        anim->setDuration(200);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        // Show menubar if auto hidden
        if (!m_glowBar->isVisible()) {
            m_glowBar->show();
        }
    }
    m_prevCursorPos = cursorPos;
}

void TopMenuBar::slotHideGlowBar()
{
    if (m_prevCursorPos == QCursor::pos()) {
       hideGlowBar();
    } else {
        m_hideGlowTimer->start(10000);
    }
}

void TopMenuBar::showGlowBar()
{
    if (m_glowBar) {
        m_hideGlowTimer->start(10000);
        m_glowBar->setWindowOpacity(glowBarOpacity());
        m_glowBar->show();
    }
}

void TopMenuBar::hideGlowBar()
{
    if (m_glowBar) {
        m_glowBar->hide();
    }
}

qreal TopMenuBar::glowBarOpacity()
{
    QPoint cursorPos = QCursor::pos();
    QDesktopWidget *desktop = QApplication::desktop();
    int screen = desktop->screenNumber(cursorPos);
    QRect desktopRect = desktop->availableGeometry(screen);
    return 1.0 - ((cursorPos.y() - desktopRect.y())/qreal(desktopRect.height())*2.0);
}

QRect TopMenuBar::triggerRect()
{
    QPoint triggerPoint = QPoint(x(), y());
    QSize triggerSize = QSize(sizeHint().width(), 5);
    return QRect(triggerPoint, triggerSize);
}

#include "topmenubar.moc"