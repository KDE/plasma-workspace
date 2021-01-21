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

#include "verticalmenu.h"

#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>

VerticalMenu::VerticalMenu(QWidget *parent)
    : QMenu(parent)
{
}

VerticalMenu::~VerticalMenu()
{
}

QMenu *VerticalMenu::leafMenu()
{
    QMenu *leaf = this;
    while (true) {
        QAction *act = leaf->activeAction();
        if (act && act->menu() && act->menu()->isVisible()) {
            leaf = act->menu();
            continue;
        }
        return leaf == this ? nullptr : leaf;
    }
    return nullptr; // make gcc happy
}

void VerticalMenu::paintEvent(QPaintEvent *pe)
{
    QMenu::paintEvent(pe);
    if (QWidget::mouseGrabber() == this)
        return;
    if (QWidget::mouseGrabber())
        QWidget::mouseGrabber()->releaseMouse();
    grabMouse();
    grabKeyboard();
}

#define FORWARD(_EVENT_, _TYPE_)                                                                                                                               \
    void VerticalMenu::_EVENT_##Event(Q##_TYPE_##Event *e)                                                                                                     \
    {                                                                                                                                                          \
        if (QMenu *leaf = leafMenu())                                                                                                                          \
            QCoreApplication::sendEvent(leaf, e);                                                                                                              \
        else                                                                                                                                                   \
            QMenu::_EVENT_##Event(e);                                                                                                                          \
    }

FORWARD(keyPress, Key)
FORWARD(keyRelease, Key)
