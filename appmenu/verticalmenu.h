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

#ifndef VERTICALMENU_H
#define VERTICALMENU_H

#include <QMenu>

class VerticalMenu : public QMenu
{
Q_OBJECT
public:
    VerticalMenu(QWidget * parent = 0);
    ~VerticalMenu() override;

    /**
     * Set menu parent window id
     */
    void setParentWid(WId id) { m_wid = id; }
    /**
     * Get menu parent window id
     */
    WId parentWid() { return m_wid; }
protected:
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void paintEvent(QPaintEvent*) override;
private:
    QMenu *leafMenu();
private:
    WId m_wid;
};

#endif //VERTICALMENU_H
