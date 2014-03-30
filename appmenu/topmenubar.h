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

#ifndef TOPMENUBAR__H
#define TOPMENUBAR__H

#include "menubar.h"

class QTimer;
class GlowBar;

class TopMenuBar : public MenuBar
{
Q_OBJECT
public:
    TopMenuBar();
    ~TopMenuBar();

    /**
     * Start mouse tracking (hide/show on mouse event)
     */
    void enableMouseTracking(bool enable = true);

    /**
     * Set menubar parent window id
     */
    void setParentWid(WId id) { m_wid = id; }
    /**
     * Get menubar parent window id
     */
    WId parentWid() { return m_wid; }
    /**
     * resize menu bar to feet content
     */
    void updateSize();
    /**
     * Move menubar and glow bar at position
     */
    void move(QPoint p);
protected:
    bool cursorInMenuBar();
private Q_SLOTS:
    void slotAboutToHide();
    void slotMouseTracker();
    void slotHideGlowBar();
private:
    void showGlowBar();
    void hideGlowBar();
    qreal glowBarOpacity();
    QRect triggerRect();

    WId m_wid;
    QPoint m_prevCursorPos;
    QTimer* m_mouseTracker;
    QTimer* m_hideGlowTimer;
    GlowBar* m_glowBar;
};

#endif
